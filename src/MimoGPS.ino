SYSTEM_MODE(MANUAL)

#include "AssetTracker2.h"

int transmittingData = 1;

// Used to keep track of the last time we published data
long lastPublish = 0;

// Minutes between publishing
unsigned int delayMinutes = 1;

// Creating an AssetTracker named 't' for us to reference
AssetTracker2 gps = AssetTracker2();

// A FuelGauge named 'fuel' for checking on the battery state
FuelGauge fuel;

String gpsPosition = "Not fixed";

// All the data in one string
// "Lat, Lng, Alt, Acc, MMS"
String buf;

int transmitMode(String command);
int gpsPublish(String command);
int batteryStatus(String command);

void setup() {
    // initializes the acceleration detector
    gps.begin();

    // Opens up a Serial port so you can listen over USB
    Serial.begin(9600);

    // Wait to allow particle serial monitor to get connected
    delay(10000);

    // enabling gps while cell service is connecting doesn't work because
    // of conflict for cpu time.
    // Turn on gps so then turn on cell service. Doing it in the reverse
    // order doesn't work

    // Enable the GPS module. 
    gps.gpsOn();

    // These functions are useful for remote diagnostics. Read more below.
    // These can be registered before connecting
    Particle.function("tmode", transmitMode);
    Particle.function("batt", batteryStatus);

    Particle.variable("pos", &gpsPosition, STRING);

    Particle.connect();

    // You can also register functions after connection
    Particle.function("gpsRate", gpsRate);
    Particle.function("gpsPublish", gpsPublish);
}

void loop() {
    // You'll need to run this every loop to capture the GPS output
    gps.updateGPS();

    // is it time to publish?
    if (millis() - lastPublish > delayMinutes * 60 * 1000) {
        // Remember when we published
        lastPublish = millis();

        Serial.println(gpsPosition);

        // we should only publish data if there's a fix
        if (gps.gpsFix()) {
            // Only publish if we're in transmittingData mode 1;
            if (transmittingData) {
                buf = String::format(
                    "{Lat: %f, Lng: %f, Alt: %d, Acc: %d, MMS: %d}",
                    gps.readLatDeg(),
                    gps.readLonDeg(),
                    gps.getAltitude(),
                    gps.getHaccuracy(),
                    gps.getSpeed()
                );

                Particle.publish("loc", buf, 60, PRIVATE);
                gpsPosition = buf;
            }
            // report the data over serial
            Serial.println(gps.readLatLon());
        }
    }
}

// Allows you to remotely change whether a device is publishing to the cloud
// or is only reporting data over Serial. Saves data when using only Serial!
// Change the default at the top of the code.
int transmitMode(String command) {
    transmittingData = atoi(command);
    return 1;
}

// Allows changing the measurement rate
int gpsRate(String command) {
    uint16_t rate = atoi(command);
    int nav = atoi(command.substring(command.indexOf(' ')));
    Serial.print("rate: ");
    Serial.print(rate);
    Serial.print(" nav: ");
    Serial.println(nav);
    gps.gpsRate(rate, nav);
    return 1;
}

// Actively ask for a GPS reading if you're impatient. Only publishes if there's
// a GPS fix, otherwise returns '0'
int gpsPublish(String command) {
    if (gps.gpsFix()) {
        Particle.publish("G", gps.readLatLon(), 60, PRIVATE);
        return 1;
    } else {
        return 0;
    }
}
