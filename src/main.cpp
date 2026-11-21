#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_system.h>
#include "config.h"
#include "firebase_handler.h"
#include "logger.h"
#include "measurement.h"
#include "state.h"
#include "webserver.h"

static constexpr uint32_t READ_INTERVAL = 10000, SEND_INTERVAL = 60000;
static constexpr uint32_t RETRY_MIN = 5000, RETRY_MAX = 300000;
static SemaphoreHandle_t storageMutex;
static QueueHandle_t disconnectQueue;
static bool elapsed(uint32_t now, uint32_t then, uint32_t interval) { return uint32_t(now - then) >= interval; }

static void transportTask(void*) {
  uint32_t lastAttempt = 0, delayMs = SEND_INTERVAL;
  bool attempted = false;
  char deviceBuffer[32], bootBuffer[33];
  snprintf(deviceBuffer, sizeof(deviceBuffer), "ESP32-C3-%012llX", static_cast<unsigned long long>(ESP.getEfuseMac()));
  snprintf(bootBuffer, sizeof(bootBuffer), "%08lX%08lX%08lX%08lX", (unsigned long)esp_random(), (unsigned long)esp_random(), (unsigned long)esp_random(), (unsigned long)esp_random());
  const String deviceId(deviceBuffer), bootId(bootBuffer);
  Serial.printf("[Schema] Device: %s\n", deviceId.c_str());
  DeviceStatus pending{};
  String pendingId;
  bool schemaPending = false, registered = false, schemaAttempted = false;
  uint32_t lastSchemaAttempt = 0;
  for (;;) {
    bool event;
    while (xQueueReceive(disconnectQueue, &event, 0) == pdTRUE) {
      xSemaphoreTake(storageMutex, portMAX_DELAY); Logger::logWiFiDisconnect(); xSemaphoreGive(storageMutex);
    }
    DeviceStatus s = getStatus();
    uint32_t now = millis();
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && s.hasReading && (!attempted || elapsed(now, lastAttempt, delayMs))) {
      lastAttempt = now; attempted = true;
      s.transportBusy = true; s.hasAttempt = true; s.lastAttempt = now; s.nextAttemptIn = 0; updateTransport(s);
      if (!checkFirebaseConnection()) initFirebase();
      bool ok = checkFirebaseConnection() && sendVoltageToFirebase(s.voltage, s.raw);
      if (ok && xSemaphoreTake(storageMutex, 0) == pdTRUE) {
        if (Logger::hasPendingLogs()) sendLogsToFirebase();
        xSemaphoreGive(storageMutex);
      }
      now = millis(); lastAttempt = now;
      s = getStatus(); s.transportBusy = false; s.token = checkFirebaseConnection();
      if (ok) { s.hasSuccess = true; s.lastSuccess = now; delayMs = SEND_INTERVAL; }
      else { delayMs = delayMs == SEND_INTERVAL ? RETRY_MIN : min(delayMs * 2, RETRY_MAX); }
      updateTransport(s);
    }
    s = getStatus();
    now = millis();
    if (connected && checkFirebaseConnection() && (!schemaAttempted || elapsed(now, lastSchemaAttempt, SEND_INTERVAL))) {
      schemaAttempted = true; lastSchemaAttempt = now;
      if (!registered) registered = registerVoltageSensor(deviceId);
      if (!schemaPending && s.hasReading && s.measuredAtSeconds != 0) {
        pending = s;
        pendingId = bootId + "-" + String(s.generation);
        schemaPending = true;
      }
      if (registered && schemaPending && sendSchemaVoltage(deviceId, pendingId, pending.voltage, pending.raw, pending.measuredAtSeconds)) schemaPending = false;
    }
    s = getStatus(); s.token = checkFirebaseConnection();
    now = millis();
    s.nextAttemptIn = connected && attempted && !elapsed(now, lastAttempt, delayMs) ? delayMs - uint32_t(now - lastAttempt) : 0;
    updateTransport(s);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
static void setupAccessPoint() {
  WiFi.mode(WIFI_AP_STA);
  IPAddress ip(192,168,4,1), mask(255,255,255,0);
  WiFi.softAPConfig(ip, ip, mask);
  WiFi.softAP("ESP32-VoltageLog", "12345678", 1, false, 4);
}
void setup() {
  Serial.begin(115200);
  storageMutex = xSemaphoreCreateMutex(); disconnectQueue = xQueueCreate(8, sizeof(bool));
  initEEPROM(); Logger::init(); initMeasurement();
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  WiFi.persistent(false); WiFi.setAutoReconnect(true);
  WiFiConfig config{};
  if (hasValidWiFiConfig()) { loadWiFiConfig(config); WiFi.mode(WIFI_STA); WiFi.begin(config.ssid, config.password); }
  else setupAccessPoint();
  setupWebServer();
  xTaskCreate(transportTask, "transport", 12288, nullptr, 1, nullptr);
}
void loop() {
  static uint32_t lastRead = 0, lastReconnect = 0, restartAt = 0;
  static bool readOnce = false, wasConnected = false, restarting = false, configPending = false;
  static WiFiConfig config{};
  uint32_t now = millis();
  bool connected = WiFi.status() == WL_CONNECTED;
  if (wasConnected && !connected) { bool event = true; xQueueSend(disconnectQueue, &event, 0); }
  wasConnected = connected;
  if (!connected && WiFi.getMode() == WIFI_STA && elapsed(now, lastReconnect, 30000)) { lastReconnect = now; WiFi.reconnect(); }
  updateWifi(connected);
  if (!readOnce || elapsed(now, lastRead, READ_INTERVAL)) {
    lastRead = now; readOnce = true; Reading reading = measureVoltage();
    time_t sampledAt = time(nullptr);
    updateMeasurement(reading.voltage, reading.raw, now, sampledAt >= 1577836800 ? static_cast<uint32_t>(sampledAt) : 0);
    Serial.printf("ADC: %d, voltage: %.3f V\n", reading.raw, reading.voltage);
  }
  if (!restarting && !configPending) configPending = takeConfig(config);
  if (configPending && xSemaphoreTake(storageMutex, 0) == pdTRUE) {
    saveWiFiConfig(config.ssid, config.password); xSemaphoreGive(storageMutex);
    configPending = false; restartAt = now; restarting = true;
  }
  if (restarting && elapsed(now, restartAt, 2000)) ESP.restart();
  delay(10);
}
