#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "firebase_handler.h"
#include "webserver.h"

// Pin na koji je spojen signalni pin (S/OUT) modula
// ESP32-C3: GPIO5 je na ADC2 (nije podržan). Koristi GPIO4 (ADC1, kanal 4)
const int voltageSensorPin = 4;

// Faktor skaliranja za modul 5:1
// (5V ulaz daje 1V izlaz na S pinu, 15V ulaz daje 3V izlaz)
const float voltageDividerFactor = 5.0;

// Referentni napon ESP32 ADC-a (tipično 3.3 V na C3)
const float adcReferenceVoltage = 3.3;

// Rezolucija ADC-a na ESP32 je 12 bita (0 do 4095)
const float adcResolution = 4095.0;

// Kalibracijski faktor
const float calibrationFactor = 0.91;

bool wifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 10000;    // provjera veze svakih 10 s

unsigned long lastFirebaseSend = 0;
const unsigned long FIREBASE_SEND_INTERVAL = 60000;  // slanje na Firebase svakih 60 s

void connectToWiFi(const char* ssid, const char* password) {
  Serial.println("\nPokušaj spajanja na WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);       // ne zapisuj automatski u flash
  WiFi.setAutoReconnect(true);  // automatski reconnect
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✓ Spojeno na WiFi!");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());

    // Inicijalizacija Firebase-a kad je WiFi gore
    initFirebase();
  } else {
    wifiConnected = false;
    Serial.println("\n✗ Nije moguće spojiti se na WiFi!");
  }
}

void setupAccessPoint() {
  Serial.println("Pokretanje Access Point moda...");

  // AP + STA mod da možemo skenirati mreže
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
  Serial.println("\n\n=== ESP32 VoltageLog - Pokretanje ===");

  // Inicijalizacija EEPROM-a
  initEEPROM();

  // Postavke ADC-a za bolju preciznost
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(voltageSensorPin, INPUT);

  // Provjera je li WiFi konfiguracija pohranjena
  if (hasValidWiFiConfig()) {
    WiFiConfig config;
    loadWiFiConfig(config);
    connectToWiFi(config.ssid, config.password);
  } else {
    Serial.println("Nema pohrane WiFi kredencijala!");
    Serial.println("Kreiram Access Point za konfiguraciju...");
    setupAccessPoint();
  }

  // Postavi web server (rute se dodaju samo jednom)
  setupWebServer();
}

void loop() {
  // Provjera WiFi veze ako je trenutno spojena
  if (wifiConnected && millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      Serial.println("WiFi veza prekinuta!");
      setupAccessPoint();
      setupWebServer();  // neće duplo registrirati rute
    }
    lastWiFiCheck = millis();
  }

  // Ako nije spojeno na WiFi i još nismo u AP/AP_STA modu, pokreni AP
  if (!wifiConnected && WiFi.getMode() == WIFI_STA) {
    setupAccessPoint();
  }

  // AsyncWebServer se brine sam, ali ostavljam hook radi jasnoće
  handleWebServer();

  // 1. Očitavanje analogne vrijednosti (0 do 4095)
  int rawValue = analogRead(voltageSensorPin);

  // 2. Izračun napona na S pinu modula
  float measuredVoltageADC =
      (rawValue * (adcReferenceVoltage / adcResolution)) * calibrationFactor;

  // 3. Izračun stvarnog ulaznog napona koristeći faktor djelitelja
  float actualInputVoltage = measuredVoltageADC * voltageDividerFactor;

  // Ispis rezultata na serijski monitor
  Serial.print("Ocitana sirova vrijednost: ");
  Serial.print(rawValue);
  Serial.print(" -> Napon ADC: ");
  Serial.print(measuredVoltageADC);
  Serial.print(" V -> Izmjereni napon: ");
  Serial.print(actualInputVoltage);
  Serial.println(" V");

  // Slanje na Firebase ako je WiFi spojen
  if (wifiConnected && (millis() - lastFirebaseSend > FIREBASE_SEND_INTERVAL)) {
    if (sendVoltageToFirebase(actualInputVoltage, rawValue)) {
      lastFirebaseSend = millis();
    }
  }

  delay(10000); // Očitava svakih 10 sekundi
}
