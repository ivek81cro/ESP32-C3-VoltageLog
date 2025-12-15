#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoJson.h>
#include "webserver.h"
#include "config.h"
#include "ui/index_html.h"
#include "ui/styles_css.h"
#include "ui/script_js.h"
#include "ui/index_html.h"
#include "ui/styles_css.h"
#include "ui/script_js.h"

// Global server instance
AsyncWebServer server(80);

// Flag to start routes and server only once
static bool serverSetup = false;

// HTML builder function - combines HTML template with CSS and JS from PROGMEM
String buildHtmlPage() {
  String html = "";
  
  // Read entire HTML from PROGMEM
  int htmlLen = strlen_P(indexHtml);
  for (int i = 0; i < htmlLen; i++) {
    char c = pgm_read_byte(indexHtml + i);
    
    // Replace CSS placeholder
    if (strncmp_P(&indexHtml[i], PSTR("<!--CSS_PLACEHOLDER-->"), 22) == 0) {
      int cssLen = strlen_P(stylesCss);
      for (int j = 0; j < cssLen; j++) {
        html += (char)pgm_read_byte(stylesCss + j);
      }
      i += 21;  // Skip past placeholder
      continue;
    }
    
    // Replace JS placeholder
    if (strncmp_P(&indexHtml[i], PSTR("<!--JS_PLACEHOLDER-->"), 21) == 0) {
      int jsLen = strlen_P(scriptJs);
      for (int j = 0; j < jsLen; j++) {
        html += (char)pgm_read_byte(scriptJs + j);
      }
      i += 20;  // Skip past placeholder
      continue;
    }
    
    html += c;
  }
  
  return html;
}

void setupWebServer() {
  Serial.println("Setting up web server...");

  if (!serverSetup) {
    // Main page - any method (GET, HEAD...)
    server.on("/", [](AsyncWebServerRequest *request) {
      Serial.print("Request: ");
      Serial.println(request->url());
      String htmlContent = buildHtmlPage();
      request->send(200, "text/html", htmlContent);
    });

    // /status - JSON status endpoint
    server.on("/status", [](AsyncWebServerRequest *request) {
      Serial.println("Request /status");
      
      // Create JSON status response
      DynamicJsonDocument doc(512);
      doc["device"] = "ESP32-C3-VoltageLog";
      doc["version"] = deviceStatus.version;
      doc["uptime"] = millis() / 1000;  // uptime in seconds
      
      // Voltage readings
      JsonObject voltage = doc.createNestedObject("voltage");
      voltage["current"] = deviceStatus.lastVoltage;
      voltage["raw"] = deviceStatus.lastRawValue;
      voltage["unit"] = "V";
      
      // WiFi status
      JsonObject wifi = doc.createNestedObject("wifi");
      wifi["connected"] = wifiConnected;
      wifi["ssid"] = wifiConnected ? WiFi.SSID().c_str() : "";
      wifi["ip"] = wifiConnected ? WiFi.localIP().toString().c_str() : "";
      wifi["rssi"] = wifiConnected ? WiFi.RSSI() : 0;
      
      // Firebase status
      JsonObject firebase = doc.createNestedObject("firebase");
      firebase["connected"] = deviceStatus.firebaseConnected;
      firebase["lastSend"] = deviceStatus.lastSendTime;
      
      // Readings timing
      JsonObject timing = doc.createNestedObject("timing");
      timing["lastRead"] = deviceStatus.lastReadTime;
      timing["lastSend"] = deviceStatus.lastSendTime;
      timing["nextSendIn"] = "60s";  // hardcoded for now
      
      String response;
      serializeJson(doc, response);
      
      request->send(200, "application/json", response);
      String htmlContent = buildHtmlPage();
      request->send(200, "text/html", htmlContent);
    });

    // /status - JSON status endpoint
    server.on("/status", [](AsyncWebServerRequest *request) {
      Serial.println("Request /status");
      
      // Create JSON status response
      DynamicJsonDocument doc(512);
      doc["device"] = "ESP32-C3-VoltageLog";
      doc["version"] = deviceStatus.version;
      doc["uptime"] = millis() / 1000;  // uptime in seconds
      
      // Voltage readings
      JsonObject voltage = doc.createNestedObject("voltage");
      voltage["current"] = deviceStatus.lastVoltage;
      voltage["raw"] = deviceStatus.lastRawValue;
      voltage["unit"] = "V";
      
      // WiFi status
      JsonObject wifi = doc.createNestedObject("wifi");
      wifi["connected"] = wifiConnected;
      wifi["ssid"] = wifiConnected ? WiFi.SSID().c_str() : "";
      wifi["ip"] = wifiConnected ? WiFi.localIP().toString().c_str() : "";
      wifi["rssi"] = wifiConnected ? WiFi.RSSI() : 0;
      
      // Firebase status
      JsonObject firebase = doc.createNestedObject("firebase");
      firebase["connected"] = deviceStatus.firebaseConnected;
      firebase["lastSend"] = deviceStatus.lastSendTime;
      
      // Readings timing
      JsonObject timing = doc.createNestedObject("timing");
      timing["lastRead"] = deviceStatus.lastReadTime;
      timing["lastSend"] = deviceStatus.lastSendTime;
      timing["nextSendIn"] = "60s";  // hardcoded for now
      
      String response;
      serializeJson(doc, response);
      
      request->send(200, "application/json", response);
    });

    // /config – save SSID / password, accepts any method (form sends POST)
    server.on("/config", [](AsyncWebServerRequest *request) {
      Serial.println("\n=== POST /config ===");

      if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password length: ");
        Serial.println(password.length());

        // Save to EEPROM
        saveWiFiConfig(ssid.c_str(), password.c_str());
        
        // Verify if saved correctly
        WiFiConfig testConfig;
        loadWiFiConfig(testConfig);
        Serial.print("Verification - SSID from EEPROM: ");
        Serial.println(testConfig.ssid);

        // Send response
        request->send(200, "text/plain", "OK! Rebooting...");

        // Wait a bit before restart
        delay(2000);
        ESP.restart();
      } else {
        Serial.println("Missing SSID or password");
        request->send(400, "text/plain", "Missing params");
      }
    });

    // /scan – network scanning
    server.on("/scan", [](AsyncWebServerRequest *request) {
      Serial.println("Request /scan");

      int n = WiFi.scanNetworks(false, false);  // blocking scan

      String json = "[";
      for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        String ssid = WiFi.SSID(i);
        int rssi   = WiFi.RSSI(i);

        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");

        json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi) + "}";
      }
      json += "]";

      Serial.print("Networks found: ");
      Serial.println(n);

      request->send(200, "application/json", json);
      WiFi.scanDelete();
    });

    // 404 handler – if root or /index.html, return main page
    server.onNotFound([](AsyncWebServerRequest *request) {
      Serial.print("Not found: ");
      Serial.println(request->url());

      if (request->url() == "/" || request->url() == "/index.html") {
        String htmlContent = buildHtmlPage();
        request->send(200, "text/html", htmlContent);
        String htmlContent = buildHtmlPage();
        request->send(200, "text/html", htmlContent);
      } else {
        request->send(404, "text/plain", "Not found");
      }
    });

    server.begin();
    serverSetup = true;
    Serial.println("Web server started on port 80");
  } else {
    Serial.println("Web server already running");
  }
}

void handleWebServer() {
  // AsyncWebServer handles connections itself, nothing needed here
}
