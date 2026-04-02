/**
 * SynoLinks Arduino Library - Implementation
 */

#include "SynoLinks.h"

// Static members
SynoWriteHandler SynoLinks::_writeHandlers[SYNOLINKS_MAX_PINS] = {nullptr};
bool SynoLinks::_handlersInitialized = false;
SynoLinks* SynoLinks::_instance = nullptr;

SynoLinks::SynoLinks(const char* authToken, const char* orgId,
                     const char* server, uint16_t port)
  : _authToken(authToken),
    _orgId(orgId),
    _server(server),
    _port(port),
    _ssid(nullptr),
    _password(nullptr),
    _mqttClient(_wifiClient),
    _lastReconnectAttempt(0),
    _lastHeartbeat(0),
    _otaEnabled(false),
    _batchCount(0)
{
  _instance = this;
  if (!_handlersInitialized) {
    memset(_writeHandlers, 0, sizeof(_writeHandlers));
    _handlersInitialized = true;
  }
}

void SynoLinks::begin(const char* ssid, const char* password) {
  _ssid = ssid;
  _password = password;

  Serial.println();
  Serial.println(F("    ____                    __    _       __       "));
  Serial.println(F("   / __/__  ______  ____  / /   (_)___  / /______ "));
  Serial.println(F("   \\__ \\/ / / / __ \\/ __ \\/ /   / / __ \\/ //_/ ___/"));
  Serial.println(F("  ___/ / /_/ / / / / /_/ / /___/ / / / / ,< (__  ) "));
  Serial.println(F(" /____/\\__, /_/ /_/\\____/_____/_/_/ /_/_/|_/____/  "));
  Serial.println(F("      /____/                                      "));
  Serial.print(F(" v"));
  Serial.println(SYNOLINKS_VERSION);
  Serial.println();

  _buildTopics();
  _connectWiFi();

  _mqttClient.setServer(_server, _port);
  _mqttClient.setCallback(_mqttCallback);
  _mqttClient.setBufferSize(1024);

  _connectMQTT();
}

void SynoLinks::run() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    _connectWiFi();
  }

  // Check MQTT
  if (!_mqttClient.connected()) {
    unsigned long now = millis();
    if (now - _lastReconnectAttempt > SYNOLINKS_RECONNECT_INTERVAL) {
      _lastReconnectAttempt = now;
      if (_connectMQTT()) {
        _lastReconnectAttempt = 0;
      }
    }
  } else {
    _mqttClient.loop();
  }

  // Heartbeat
  unsigned long now = millis();
  if (now - _lastHeartbeat > SYNOLINKS_HEARTBEAT_INTERVAL) {
    _lastHeartbeat = now;
    sendStatus();
  }
}

bool SynoLinks::connected() {
  return _mqttClient.connected();
}

// ===== Virtual Pin Write =====

void SynoLinks::virtualWrite(uint8_t pin, int value) {
  char topic[128];
  snprintf(topic, sizeof(topic), "blynk/%s/%s/up/ds/%d", _orgId, _authToken, pin);

  char payload[64];
  snprintf(payload, sizeof(payload), "{\"v\":%d,\"ts\":%lu}", value, (unsigned long)(millis()/1000));

  _mqttClient.publish(topic, payload);
}

void SynoLinks::virtualWrite(uint8_t pin, float value) {
  char topic[128];
  snprintf(topic, sizeof(topic), "blynk/%s/%s/up/ds/%d", _orgId, _authToken, pin);

  char payload[64];
  char valStr[16];
  dtostrf(value, 1, 2, valStr);
  snprintf(payload, sizeof(payload), "{\"v\":%s,\"ts\":%lu}", valStr, (unsigned long)(millis()/1000));

  _mqttClient.publish(topic, payload);
}

void SynoLinks::virtualWrite(uint8_t pin, double value) {
  virtualWrite(pin, (float)value);
}

void SynoLinks::virtualWrite(uint8_t pin, const char* value) {
  char topic[128];
  snprintf(topic, sizeof(topic), "blynk/%s/%s/up/ds/%d", _orgId, _authToken, pin);

  char payload[256];
  snprintf(payload, sizeof(payload), "{\"v\":\"%s\",\"ts\":%lu}", value, (unsigned long)(millis()/1000));

  _mqttClient.publish(topic, payload);
}

void SynoLinks::virtualWrite(uint8_t pin, const String& value) {
  virtualWrite(pin, value.c_str());
}

// ===== Batch Write =====

void SynoLinks::batchBegin() {
  _batchCount = 0;
}

void SynoLinks::batchAdd(uint8_t pin, int value) {
  if (_batchCount >= SYNOLINKS_MAX_BATCH) return;
  _batchBuffer[_batchCount].pin = pin;
  snprintf(_batchBuffer[_batchCount].value, 32, "%d", value);
  _batchCount++;
}

void SynoLinks::batchAdd(uint8_t pin, float value) {
  if (_batchCount >= SYNOLINKS_MAX_BATCH) return;
  _batchBuffer[_batchCount].pin = pin;
  dtostrf(value, 1, 2, _batchBuffer[_batchCount].value);
  _batchCount++;
}

void SynoLinks::batchAdd(uint8_t pin, const char* value) {
  if (_batchCount >= SYNOLINKS_MAX_BATCH) return;
  _batchBuffer[_batchCount].pin = pin;
  strncpy(_batchBuffer[_batchCount].value, value, 31);
  _batchBuffer[_batchCount].value[31] = '\0';
  _batchCount++;
}

void SynoLinks::batchSend() {
  if (_batchCount == 0) return;

  char topic[128];
  snprintf(topic, sizeof(topic), "blynk/%s/%s/up/batch", _orgId, _authToken);

  // Build JSON: {"d":[{"p":0,"v":23.5},{"p":1,"v":65}],"ts":123}
  JsonDocument doc;
  JsonArray arr = doc["d"].to<JsonArray>();

  for (uint8_t i = 0; i < _batchCount; i++) {
    JsonObject item = arr.add<JsonObject>();
    item["p"] = _batchBuffer[i].pin;

    // Try to parse as number first
    char* end;
    double numVal = strtod(_batchBuffer[i].value, &end);
    if (*end == '\0' && end != _batchBuffer[i].value) {
      item["v"] = numVal;
    } else {
      item["v"] = _batchBuffer[i].value;
    }
  }
  doc["ts"] = millis() / 1000;

  char payload[512];
  serializeJson(doc, payload, sizeof(payload));

  _mqttClient.publish(topic, payload);
  _batchCount = 0;
}

// ===== Status =====

void SynoLinks::sendStatus() {
  if (!_mqttClient.connected()) return;

  JsonDocument doc;
  doc["online"] = true;
  doc["fw"] = SYNOLINKS_VERSION;
  doc["rssi"] = WiFi.RSSI();
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["ip"] = WiFi.localIP().toString();

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));

  _mqttClient.publish(_topicStatus, payload);
}

void SynoLinks::terminal(uint8_t pin, const String& message) {
  virtualWrite(pin, message);
}

// ===== OTA =====

void SynoLinks::enableOTA() {
  _otaEnabled = true;
}

// ===== Internal =====

void SynoLinks::_connectWiFi() {
  Serial.print(F("[SynoLinks] Connecting to WiFi: "));
  Serial.println(_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("[SynoLinks] WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\n[SynoLinks] WiFi connection failed!"));
  }
}

bool SynoLinks::_connectMQTT() {
  Serial.print(F("[SynoLinks] Connecting to MQTT: "));
  Serial.print(_server);
  Serial.print(":");
  Serial.println(_port);

  // Use auth token as MQTT username
  if (_mqttClient.connect(_authToken, _authToken, "")) {
    Serial.println(F("[SynoLinks] MQTT connected!"));

    // Subscribe to downlink topics
    char subTopic[128];
    snprintf(subTopic, sizeof(subTopic), "blynk/%s/%s/down/#", _orgId, _authToken);
    _mqttClient.subscribe(subTopic);

    // Send initial status
    sendStatus();
    return true;
  }

  Serial.print(F("[SynoLinks] MQTT failed, rc="));
  Serial.println(_mqttClient.state());
  return false;
}

void SynoLinks::_buildTopics() {
  snprintf(_topicUp,     sizeof(_topicUp),     "blynk/%s/%s/up/ds/", _orgId, _authToken);
  snprintf(_topicDown,   sizeof(_topicDown),   "blynk/%s/%s/down/ds/", _orgId, _authToken);
  snprintf(_topicStatus, sizeof(_topicStatus), "blynk/%s/%s/up/status", _orgId, _authToken);
  snprintf(_topicOTA,    sizeof(_topicOTA),    "blynk/%s/%s/down/ota", _orgId, _authToken);
}

void SynoLinks::_mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (_instance) {
    _instance->_onMessage(topic, payload, length);
  }
}

void SynoLinks::_onMessage(char* topic, byte* payload, unsigned int length) {
  // Null-terminate payload
  char msg[512];
  unsigned int copyLen = length < 511 ? length : 511;
  memcpy(msg, payload, copyLen);
  msg[copyLen] = '\0';

  // Check if OTA command
  if (strstr(topic, "/down/ota") != nullptr) {
    if (_otaEnabled) {
      _handleOTACommand(msg, copyLen);
    }
    return;
  }

  // Parse pin number from topic: blynk/{org}/{token}/down/ds/{pin}
  char* dsPos = strstr(topic, "/down/ds/");
  if (dsPos == nullptr) return;

  int pin = atoi(dsPos + 9); // Skip "/down/ds/"
  if (pin < 0 || pin >= SYNOLINKS_MAX_PINS) return;

  // Parse value from JSON: {"v": ...}
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return;

  String valueStr;
  if (doc["v"].is<const char*>()) {
    valueStr = doc["v"].as<const char*>();
  } else if (doc["v"].is<double>()) {
    valueStr = String(doc["v"].as<double>(), 6);
  } else if (doc["v"].is<int>()) {
    valueStr = String(doc["v"].as<int>());
  }

  _handleDownlink(pin, valueStr.c_str());
}

void SynoLinks::_handleDownlink(int pin, const char* value) {
  if (pin >= 0 && pin < SYNOLINKS_MAX_PINS && _writeHandlers[pin] != nullptr) {
    SynoLinksParam param(value);
    _writeHandlers[pin](param);
  }
}

void SynoLinks::_handleOTACommand(const char* payload, unsigned int length) {
#if SYNOLINKS_HAS_OTA
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;

  const char* url = doc["url"];
  const char* sha256 = doc["sha256"];
  long fileSize = doc["size"] | 0;

  if (!url) {
    Serial.println(F("[SynoLinks] OTA: no URL in command"));
    return;
  }

  Serial.println(F("[SynoLinks] OTA update starting..."));
  Serial.print(F("[SynoLinks] URL: "));
  Serial.println(url);
  Serial.print(F("[SynoLinks] Size: "));
  Serial.println(fileSize);

  // Report OTA status
  char statusTopic[128];
  snprintf(statusTopic, sizeof(statusTopic), "blynk/%s/%s/up/ota", _orgId, _authToken);
  _mqttClient.publish(statusTopic, "{\"status\":\"downloading\"}");

#if defined(ESP32)
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();

    if (Update.begin(contentLength)) {
      _mqttClient.publish(statusTopic, "{\"status\":\"installing\"}");

      WiFiClient* stream = http.getStreamPtr();
      size_t written = Update.writeStream(*stream);

      if (written == (size_t)contentLength) {
        if (Update.end(true)) {
          _mqttClient.publish(statusTopic, "{\"status\":\"success\"}");
          Serial.println(F("[SynoLinks] OTA success! Rebooting..."));
          delay(1000);
          ESP.restart();
        }
      }
      _mqttClient.publish(statusTopic, "{\"status\":\"failed\",\"error\":\"write_error\"}");
    } else {
      _mqttClient.publish(statusTopic, "{\"status\":\"failed\",\"error\":\"begin_failed\"}");
    }
  } else {
    char errMsg[128];
    snprintf(errMsg, sizeof(errMsg), "{\"status\":\"failed\",\"error\":\"http_%d\"}", httpCode);
    _mqttClient.publish(statusTopic, errMsg);
  }
  http.end();

#elif defined(ESP8266)
  WiFiClient client;
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
  if (ret == HTTP_UPDATE_OK) {
    _mqttClient.publish(statusTopic, "{\"status\":\"success\"}");
  } else {
    _mqttClient.publish(statusTopic, "{\"status\":\"failed\"}");
  }
#endif

#endif // SYNOLINKS_HAS_OTA
}

void SynoLinks::_registerWriteHandler(uint8_t pin, SynoWriteHandler handler) {
  if (pin < SYNOLINKS_MAX_PINS) {
    _writeHandlers[pin] = handler;
  }
}
