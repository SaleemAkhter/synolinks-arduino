/**
 * SynoLinks - DHT11 Temperature & Humidity
 *
 * Sends temperature on V0 and humidity on V1.
 * Uses batch write to send both in a single MQTT message.
 *
 * Hardware:
 *   - ESP32 or ESP8266
 *   - DHT11 sensor on pin D4 (GPIO 4)
 *
 * Dashboard: Add "Temp & Humidity" widget linked to V0+V1
 *
 * Required library: DHT sensor library by Adafruit
 */

#include <SynoLinks.h>
#include <DHT.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

#define DHTPIN    4
#define DHTTYPE   DHT11

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  syno.begin(ssid, pass);
}

void loop() {
  syno.run();

  // Read sensor every 2 seconds
  if (millis() - lastRead > 2000) {
    lastRead = millis();

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (!isnan(temp) && !isnan(hum)) {
      // Send both values in one MQTT message (efficient!)
      syno.batchBegin();
      syno.batchAdd(V0, temp);
      syno.batchAdd(V1, hum);
      syno.batchSend();

      Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n", temp, hum);
    }
  }
}
