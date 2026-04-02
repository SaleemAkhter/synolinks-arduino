/**
 * SynoLinks - HC-SR04 Ultrasonic Distance Sensor
 *
 * Sends distance (cm) on V0.
 * Dashboard: Add "Distance Meter" widget linked to V0
 *
 * Hardware:
 *   - ESP32 or ESP8266
 *   - HC-SR04: TRIG -> GPIO 5, ECHO -> GPIO 18
 */

#include <SynoLinks.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

#define TRIG_PIN  5
#define ECHO_PIN  18

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);

unsigned long lastRead = 0;

float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2.0;  // cm
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  syno.begin(ssid, pass);
}

void loop() {
  syno.run();

  if (millis() - lastRead > 500) {
    lastRead = millis();

    float distance = readDistance();
    if (distance > 0 && distance < 400) {
      syno.virtualWrite(V0, distance);
      Serial.printf("Distance: %.1f cm\n", distance);
    }
  }
}
