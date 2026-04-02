/**
 * SynoLinks - Getting Started
 *
 * This example connects an ESP32/ESP8266 to SynoLinks platform
 * and sends a counter value every second on Virtual Pin V0.
 *
 * Steps:
 *   1. Create a device in SynoLinks dashboard
 *   2. Copy the AUTH_TOKEN and ORG_ID
 *   3. Set your WiFi credentials below
 *   4. Upload and open Serial Monitor (115200)
 *   5. See live data in your SynoLinks dashboard!
 *
 * Hardware: ESP32 or ESP8266 (no extra components needed)
 */

#include <SynoLinks.h>

// ===== CONFIGURATION =====
#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"  // Your SynoLinks MQTT server

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);

int counter = 0;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  syno.begin(ssid, pass);
}

void loop() {
  syno.run();

  // Send counter every 1 second
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    counter++;

    syno.virtualWrite(V0, counter);
    Serial.print("Sent: V0 = ");
    Serial.println(counter);
  }
}
