/**
 * SynoLinks - OTA Firmware Update
 *
 * Enables over-the-air firmware updates from the SynoLinks dashboard.
 * Upload new firmware via Dashboard > OTA > Deploy, and the device
 * will automatically download and install it.
 *
 * Hardware: ESP32 or ESP8266 (no extra components)
 */

#include <SynoLinks.h>

#define AUTH_TOKEN  "YourDeviceAuthToken"
#define ORG_ID     "YourOrgId"
#define MQTT_HOST  "your-server.com"

char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

SynoLinks syno(AUTH_TOKEN, ORG_ID, MQTT_HOST);

void setup() {
  Serial.begin(115200);

  // Enable OTA - device will accept firmware updates from dashboard
  syno.enableOTA();

  syno.begin(ssid, pass);

  // Send current firmware version on V0
  syno.virtualWrite(V0, SYNOLINKS_VERSION);
}

void loop() {
  syno.run();
}
