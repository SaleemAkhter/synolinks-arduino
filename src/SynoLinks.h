/**
 * SynoLinks Arduino Library
 *
 * Connect your Arduino/ESP32/ESP8266 to SynoLinks IoT Platform.
 * Uses MQTT for real-time communication with virtual pins.
 *
 * Usage:
 *   #include <SynoLinks.h>
 *
 *   SynoLinks syno("AUTH_TOKEN", "ORG_ID", "mqtt.synolinks.com");
 *
 *   SYNO_WRITE(V0) {
 *     int value = param.asInt();
 *     digitalWrite(LED, value);
 *   }
 *
 *   void setup() {
 *     syno.begin(ssid, password);
 *   }
 *
 *   void loop() {
 *     syno.run();
 *     syno.virtualWrite(V1, temperature);
 *   }
 */

#ifndef SYNOLINKS_H
#define SYNOLINKS_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Platform-specific includes
#if defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <Update.h>
  #define SYNOLINKS_HAS_OTA 1
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>
  #define SYNOLINKS_HAS_OTA 1
#else
  #define SYNOLINKS_HAS_OTA 0
#endif

#include <PubSubClient.h>

#define SYNOLINKS_VERSION "1.0.0"
#define SYNOLINKS_DEFAULT_PORT 1883
#define SYNOLINKS_MAX_PINS 256
#define SYNOLINKS_MAX_BATCH 16
#define SYNOLINKS_RECONNECT_INTERVAL 5000
#define SYNOLINKS_HEARTBEAT_INTERVAL 30000

// Virtual pin definitions
enum {
  V0=0, V1, V2, V3, V4, V5, V6, V7, V8, V9,
  V10, V11, V12, V13, V14, V15, V16, V17, V18, V19,
  V20, V21, V22, V23, V24, V25, V26, V27, V28, V29,
  V30, V31, V32, V33, V34, V35, V36, V37, V38, V39,
  V40, V41, V42, V43, V44, V45, V46, V47, V48, V49,
  V50, V51, V52, V53, V54, V55, V56, V57, V58, V59,
  V60, V61, V62, V63, V64, V65, V66, V67, V68, V69,
  V70, V71, V72, V73, V74, V75, V76, V77, V78, V79,
  V80, V81, V82, V83, V84, V85, V86, V87, V88, V89,
  V90, V91, V92, V93, V94, V95, V96, V97, V98, V99,
  V100, V101, V102, V103, V104, V105, V106, V107, V108, V109,
  V110, V111, V112, V113, V114, V115, V116, V117, V118, V119,
  V120, V121, V122, V123, V124, V125, V126, V127
};

/**
 * Parameter wrapper for incoming virtual pin values
 */
class SynoLinksParam {
public:
  SynoLinksParam() : _strVal(""), _numVal(0), _valid(false) {}
  SynoLinksParam(const char* val) : _strVal(val), _valid(true) {
    _numVal = atof(val);
  }
  SynoLinksParam(const String& val) : _strVal(val), _valid(true) {
    _numVal = val.toDouble();
  }

  int    asInt()    const { return (int)_numVal; }
  long   asLong()   const { return (long)_numVal; }
  float  asFloat()  const { return (float)_numVal; }
  double asDouble() const { return _numVal; }
  const String& asStr()  const { return _strVal; }
  const String& asString() const { return _strVal; }
  bool   isEmpty()  const { return !_valid || _strVal.length() == 0; }

private:
  String _strVal;
  double _numVal;
  bool   _valid;
};

// Callback type for virtual pin writes
typedef void (*SynoWriteHandler)(SynoLinksParam param);

/**
 * Macro to define a virtual pin write handler.
 * Usage:
 *   SYNO_WRITE(V0) {
 *     int val = param.asInt();
 *     // do something with val
 *   }
 */
#define SYNO_WRITE(pin) \
  void _synoWriteHandler_##pin(SynoLinksParam param); \
  struct _SynoWriteReg_##pin { \
    _SynoWriteReg_##pin() { SynoLinks::_registerWriteHandler(pin, _synoWriteHandler_##pin); } \
  } _synoWriteRegInstance_##pin; \
  void _synoWriteHandler_##pin(SynoLinksParam param)

/**
 * Main SynoLinks class - handles WiFi, MQTT, and virtual pins.
 */
class SynoLinks {
public:
  /**
   * Constructor
   * @param authToken  Device auth token from SynoLinks dashboard
   * @param orgId      Organization ID
   * @param server     MQTT server hostname (default: localhost)
   * @param port       MQTT port (default: 1883)
   */
  SynoLinks(const char* authToken, const char* orgId,
            const char* server = "localhost", uint16_t port = SYNOLINKS_DEFAULT_PORT);

  /**
   * Initialize WiFi and connect to MQTT broker.
   * @param ssid     WiFi SSID
   * @param password WiFi password
   */
  void begin(const char* ssid, const char* password);

  /**
   * Must be called in loop(). Handles MQTT keepalive, reconnection, and messages.
   */
  void run();

  /**
   * Check if connected to MQTT broker.
   */
  bool connected();

  // ===== Virtual Pin Write =====

  /** Write an integer value to a virtual pin */
  void virtualWrite(uint8_t pin, int value);

  /** Write a float value to a virtual pin */
  void virtualWrite(uint8_t pin, float value);

  /** Write a double value to a virtual pin */
  void virtualWrite(uint8_t pin, double value);

  /** Write a string value to a virtual pin */
  void virtualWrite(uint8_t pin, const char* value);

  /** Write a String value to a virtual pin */
  void virtualWrite(uint8_t pin, const String& value);

  // ===== Batch Write (multiple pins in one MQTT message) =====

  /** Start a batch write */
  void batchBegin();

  /** Add a pin value to the batch */
  void batchAdd(uint8_t pin, int value);
  void batchAdd(uint8_t pin, float value);
  void batchAdd(uint8_t pin, const char* value);

  /** Send the batch */
  void batchSend();

  // ===== Status =====

  /** Send device status (firmware version, RSSI, free heap) */
  void sendStatus();

  /** Send a terminal/debug message on a pin */
  void terminal(uint8_t pin, const String& message);

  // ===== OTA =====

  /** Enable OTA firmware updates */
  void enableOTA();

  // ===== Internal =====

  /** Register a write handler (called by SYNO_WRITE macro) */
  static void _registerWriteHandler(uint8_t pin, SynoWriteHandler handler);

private:
  const char* _authToken;
  const char* _orgId;
  const char* _server;
  uint16_t    _port;
  const char* _ssid;
  const char* _password;

  WiFiClient  _wifiClient;
  PubSubClient _mqttClient;

  unsigned long _lastReconnectAttempt;
  unsigned long _lastHeartbeat;
  bool _otaEnabled;

  // Topic buffers
  char _topicUp[128];
  char _topicDown[128];
  char _topicStatus[128];
  char _topicOTA[128];

  // Batch buffer
  struct BatchItem {
    uint8_t pin;
    char value[32];
  };
  BatchItem _batchBuffer[SYNOLINKS_MAX_BATCH];
  uint8_t _batchCount;

  // Static handler registry
  static SynoWriteHandler _writeHandlers[SYNOLINKS_MAX_PINS];
  static bool _handlersInitialized;

  void _connectWiFi();
  bool _connectMQTT();
  void _buildTopics();
  void _onMessage(char* topic, byte* payload, unsigned int length);
  void _handleDownlink(int pin, const char* value);
  void _handleOTACommand(const char* payload, unsigned int length);

  static void _mqttCallback(char* topic, byte* payload, unsigned int length);
  static SynoLinks* _instance;
};

#endif // SYNOLINKS_H
