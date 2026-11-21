#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "webserver.h"
#include "state.h"
#include "ui/index_html.h"
#include "ui/styles_css.h"
#include "ui/script_js.h"
static AsyncWebServer server(80);
static String page() {
  String html(FPSTR(indexHtml));
  html.replace("<!--CSS_PLACEHOLDER-->", FPSTR(stylesCss));
  html.replace("<!--JS_PLACEHOLDER-->", FPSTR(scriptJs));
  return html;
}
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/html", page()); });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/html", page()); });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    DeviceStatus s = getStatus();
    DynamicJsonDocument doc(768);
    doc["device"] = "ESP32-C3-VoltageLog"; doc["version"] = "phase-2-dev"; doc["uptime"] = millis()/1000;
    JsonObject voltage = doc.createNestedObject("voltage"); voltage["current"] = s.voltage; voltage["raw"] = s.raw; voltage["unit"] = "V"; voltage["valid"] = s.hasReading;
    JsonObject wifi = doc.createNestedObject("wifi"); wifi["connected"] = s.wifi;
    JsonObject firebase = doc.createNestedObject("firebase"); firebase["connected"] = s.token; firebase["tokenPresent"] = s.token; firebase["lastSend"] = s.lastSuccess; firebase["hasSuccessfulSend"] = s.hasSuccess; firebase["busy"] = s.transportBusy;
    JsonObject timing = doc.createNestedObject("timing"); timing["lastRead"] = s.lastRead; timing["lastSend"] = s.lastSuccess; timing["lastAttempt"] = s.lastAttempt; timing["hasAttempt"] = s.hasAttempt; timing["nextSendIn"] = s.nextAttemptIn / 1000;
    String output; serializeJson(doc, output); request->send(200, "application/json", output);
  });
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) { request->send(400, "text/plain", "Missing params"); return; }
    String ssid = request->getParam("ssid", true)->value(), password = request->getParam("password", true)->value();
    if (ssid.length() == 0 || ssid.length() > SSID_MAX_LEN || password.length() < 8 || password.length() > 63 || strlen(ssid.c_str()) != ssid.length() || strlen(password.c_str()) != password.length()) { request->send(400, "text/plain", "Invalid SSID or password length"); return; }
    WiFiConfig config{}; ssid.toCharArray(config.ssid, sizeof(config.ssid)); password.toCharArray(config.password, sizeof(config.password));
    if (!queueConfig(config)) { request->send(409, "text/plain", "Configuration pending"); return; }
    request->send(202, "text/plain", "Configuration queued; restarting");
  });
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true, false); request->send(202, "application/json", "{\"scanning\":true}"); return; }
    if (n == WIFI_SCAN_RUNNING) { request->send(202, "application/json", "{\"scanning\":true}"); return; }
    DynamicJsonDocument doc(8192); JsonArray entries = doc.to<JsonArray>();
    for (int i = 0; i < n && i < 32; ++i) { JsonObject ap = entries.createNestedObject(); ap["ssid"] = WiFi.SSID(i); ap["rssi"] = WiFi.RSSI(i); }
    String output; serializeJson(doc, output); WiFi.scanDelete(); request->send(200, "application/json", output);
  });
  server.onNotFound([](AsyncWebServerRequest* request) { request->send(404, "text/plain", "Not found"); });
  server.begin();
}
