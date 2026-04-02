/**
 * SynoLinks - Smart Home (Multi-sensor + Control)
 *
 * A complete smart home setup:
 *   V0 = Temperature (DHT11)
 *   V1 = Humidity (DHT11)
 *   V2 = Gas Level (MQ-2, analog)
 *   V3 = Motion Detected (PIR)
 *   V4 = Light relay control (from app)
 *   V5 = Fan relay control (from app)
 *
 * Dashboard: Add Temp&Humidity, Gas Sensor, PIR Motion, Button/Switch widgets
 *
 * Hardware:
 *   - ESP32
 *   - DHT11 on GPIO 4
 *   - MQ-2 on GPIO 34 (ADC)
 *   - PIR on GPIO 14
 *   - Relay 1 (Light) on GPIO 16
 *   - Relay 2 (Fan) on GPIO 17
 */

#include <SynoLinks.h>
#include <DHT.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

#define DHT_PIN    4
#define GAS_PIN    34
#define PIR_PIN    14
#define LIGHT_PIN  16
#define FAN_PIN    17

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);
DHT dht(DHT_PIN, DHT11);

// App controls the light and fan
SYNO_WRITE(V4) {
  digitalWrite(LIGHT_PIN, param.asInt());
  Serial.printf("Light: %s\n", param.asInt() ? "ON" : "OFF");
}

SYNO_WRITE(V5) {
  digitalWrite(FAN_PIN, param.asInt());
  Serial.printf("Fan: %s\n", param.asInt() ? "ON" : "OFF");
}

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);

  pinMode(GAS_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  dht.begin();
  syno.begin(ssid, pass);
}

void loop() {
  syno.run();

  if (millis() - lastSend > 2000) {
    lastSend = millis();

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    int   gas  = analogRead(GAS_PIN);
    int   pir  = digitalRead(PIR_PIN);

    // Send all in one batch (saves bandwidth!)
    syno.batchBegin();
    if (!isnan(temp)) syno.batchAdd(V0, temp);
    if (!isnan(hum))  syno.batchAdd(V1, hum);
    syno.batchAdd(V2, gas);
    syno.batchAdd(V3, pir);
    syno.batchSend();
  }
}
