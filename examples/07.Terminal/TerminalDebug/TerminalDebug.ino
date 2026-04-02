/**
 * SynoLinks - Terminal Widget (Bidirectional)
 *
 * Send and receive text commands via the Terminal widget.
 * Type commands in the app, see responses on the device.
 *
 * V0 = Terminal (send/receive text)
 *
 * Dashboard: Add "Terminal" widget linked to V0
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

// Receive commands from terminal widget
SYNO_WRITE(V0) {
  String cmd = param.asString();
  Serial.print("Command: ");
  Serial.println(cmd);

  // Process commands
  if (cmd == "help") {
    syno.terminal(V0, "Commands: help, status, reboot, led on, led off");
  }
  else if (cmd == "status") {
    String status = "Uptime: " + String(millis() / 1000) + "s | "
                  + "Heap: " + String(ESP.getFreeHeap()) + " | "
                  + "RSSI: " + String(WiFi.RSSI()) + "dBm";
    syno.terminal(V0, status);
  }
  else if (cmd == "reboot") {
    syno.terminal(V0, "Rebooting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }
  else if (cmd == "led on") {
    digitalWrite(LED_BUILTIN, HIGH);
    syno.terminal(V0, "LED turned ON");
  }
  else if (cmd == "led off") {
    digitalWrite(LED_BUILTIN, LOW);
    syno.terminal(V0, "LED turned OFF");
  }
  else {
    syno.terminal(V0, "Unknown command. Type 'help'");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  syno.begin(ssid, pass);
  syno.terminal(V0, "Device ready! Type 'help' for commands.");
}

void loop() {
  syno.run();
}
