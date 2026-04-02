/**
 * SynoLinks - Weather Station (Batch Data)
 *
 * Sends multiple sensor readings in a single MQTT message.
 *   V0 = Temperature (BME280)
 *   V1 = Humidity
 *   V2 = Pressure (hPa)
 *   V3 = Altitude (m)
 *
 * Dashboard: Add Data Table widget + SuperChart widget
 *
 * Hardware:
 *   - ESP32
 *   - BME280 on I2C (SDA=21, SCL=22)
 *
 * Required library: Adafruit BME280
 */

#include <SynoLinks.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);
Adafruit_BME280 bme;

unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);

  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
  }

  syno.begin(ssid, pass);
}

void loop() {
  syno.run();

  // Send every 5 seconds
  if (millis() - lastRead > 5000) {
    lastRead = millis();

    float temp     = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;  // hPa
    float altitude = bme.readAltitude(1013.25);     // m

    // Batch write - all 4 values in 1 MQTT message
    syno.batchBegin();
    syno.batchAdd(V0, temp);
    syno.batchAdd(V1, humidity);
    syno.batchAdd(V2, pressure);
    syno.batchAdd(V3, altitude);
    syno.batchSend();

    Serial.printf("T:%.1f H:%.1f P:%.1f A:%.1f\n", temp, humidity, pressure, altitude);
  }
}
