/**
 * SynoLinks - 4-Channel Relay Control
 *
 * Control 4 relays from the SynoLinks app using buttons/switches.
 * V0 = Relay 1, V1 = Relay 2, V2 = Relay 3, V3 = Relay 4
 *
 * Dashboard: Add 4 "Button" or "Switch" widgets linked to V0-V3
 *
 * Hardware:
 *   - ESP32 or ESP8266
 *   - 4-Channel Relay Module on GPIO 16, 17, 18, 19
 */

#include <SynoLinks.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);

// Relay pins
const int relayPins[] = {16, 17, 18, 19};
const int NUM_RELAYS = 4;

// Handle commands from app
SYNO_WRITE(V0) { digitalWrite(relayPins[0], param.asInt()); }
SYNO_WRITE(V1) { digitalWrite(relayPins[1], param.asInt()); }
SYNO_WRITE(V2) { digitalWrite(relayPins[2], param.asInt()); }
SYNO_WRITE(V3) { digitalWrite(relayPins[3], param.asInt()); }

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  syno.begin(ssid, pass);
}

void loop() {
  syno.run();
}
