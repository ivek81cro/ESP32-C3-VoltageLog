#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "firebase_handler.h"
#include "webserver.h"
#include "logger.h"

// Pin connected to the signal pin (S/OUT) of the module
// ESP32-C3: GPIO5 is on ADC2 (not supported). Use GPIO4 (ADC1, channel 4)
const int voltageSensorPin = 4;

// Scaling factor for 5:1 module
// (5V input gives 1V output on S pin, 15V input gives 3V output)
const float voltageDividerFactor = 5.0;

// Reference voltage of ESP32 ADC (typically 3.3 V on C3)
const float adcReferenceVoltage = 3.3;

// ADC resolution on ESP32 is 12 bits (0 to 4095)
const float adcResolution = 4095.0;

// Calibration factor
const float calibrationFactor = 0.91;

bool wifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 10000;    // check connection every 10 s

unsigned long lastFirebaseSend = 0;
const unsigned long FIREBASE_SEND_INTERVAL = 60000;  // send to Firebase every 60 s

// Status tracking variables (for /status endpoint)
DeviceStatus deviceStatus;

void connectToWiFi(const char* ssid, const char* password) {
  Serial.println("\nAttempting to connect to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);       // do not save automatically to flash
  WiFi.setAutoReconnect(true);  // auto reconnect
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✓ Connected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize Firebase when WiFi is up
    initFirebase();
    
    // Slanje logova ako postoje
    if (Logger::hasPendingLogs()) {
      Serial.println("Slanje spremljenih logova na Firebase...");
      delay(2000);  // Čekaj da se Firebase inicijalizira
      sendLogsToFirebase();
    }
  } else {
    wifiConnected = false;
    Serial.println("\n✗ Unable to connect to WiFi!");
  }
}

void setupAccessPoint() {
  Serial.println("Starting Access Point mode...");

  // AP + STA mode so we can scan networks
  WiFi.mode(WIFI_AP_STA);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress netmask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netmask);

  bool apStarted = WiFi.softAP("ESP32-VoltageLog", "12345678", 1, false, 4);

  Serial.print("Access Point Status: ");
  Serial.println(apStarted ? "STARTED" : "FAILED");

  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=== ESP32 VoltageLog - Startup ===");

  // Inicijalizacija loggera
  Logger::init();

  // Initialize EEPROM
  initEEPROM();

  // ADC settings for better precision
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(voltageSensorPin, INPUT);

  // Check if WiFi configuration is stored
  if (hasValidWiFiConfig()) {
    WiFiConfig config;
    loadWiFiConfig(config);
    connectToWiFi(config.ssid, config.password);
  } else {
    Serial.println("No stored WiFi credentials!");
    Serial.println("Creating Access Point for configuration...");
    setupAccessPoint();
  }

  // Setup web server (routes added only once)
  setupWebServer();
}

void loop() {
  // Check WiFi connection if currently connected
  if (wifiConnected && millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      Serial.println("WiFi connection lost!");
      Logger::logWiFiDisconnect();  // Logira WiFi prekid
      setupAccessPoint();
      setupWebServer();  // won't register routes twice
    }
    lastWiFiCheck = millis();
  }

  // If not connected to WiFi and not yet in AP/AP_STA mode, start AP
  if (!wifiConnected && WiFi.getMode() == WIFI_STA) {
    setupAccessPoint();
  }

  // AsyncWebServer handles itself, but leaving hook for clarity
  handleWebServer();

  // 1. Read analog value (0 to 4095)
  int rawValue = analogRead(voltageSensorPin);

  // 2. Calculate voltage on module's S pin
  float measuredVoltageADC =
      (rawValue * (adcReferenceVoltage / adcResolution)) * calibrationFactor;

  // 3. Calculate actual input voltage using divider factor
  float actualInputVoltage = measuredVoltageADC * voltageDividerFactor;

  // Print results to serial monitor
  Serial.print("Raw value read: ");
  Serial.print(rawValue);
  Serial.print(" -> ADC voltage: ");
  Serial.print(measuredVoltageADC);
  Serial.print(" V -> Measured voltage: ");
  Serial.print(actualInputVoltage);
  Serial.println(" V");

  // Update device status for /status endpoint
  deviceStatus.lastVoltage = actualInputVoltage;
  deviceStatus.lastRawValue = rawValue;
  deviceStatus.lastReadTime = millis();
  deviceStatus.firebaseConnected = checkFirebaseConnection();

  // Send to Firebase if WiFi connected
  if (wifiConnected && (millis() - lastFirebaseSend > FIREBASE_SEND_INTERVAL)) {
    if (sendVoltageToFirebase(actualInputVoltage, rawValue)) {
      lastFirebaseSend = millis();
      deviceStatus.lastSendTime = millis();
    }
  }

  delay(10000); // Read every 10 seconds
}
