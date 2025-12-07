#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "webserver.h"
#include "config.h"

// Globalna instanca servera
AsyncWebServer server(80);

// Flag da rute i server pokrećemo samo jednom
static bool serverSetup = false;

// Cijeli HTML u PROGMEM-u
const char htmlPage[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="hr">
<head>
  <meta charset="UTF-8">
  <title>ESP32 WiFi Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">

  <style>
    body {
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      margin: 0;
      padding: 0;
      background: #f3f4f6;
      color: #111827;
    }

    .container {
      max-width: 480px;
      margin: 32px auto;
      padding: 24px;
      background: #ffffff;
      border-radius: 12px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.08);
    }

    h1 {
      font-size: 1.4rem;
      margin: 0 0 4px 0;
      color: #111827;
    }

    .subtitle {
      margin: 0 0 16px 0;
      font-size: 0.9rem;
      color: #6b7280;
    }

    label {
      display: block;
      font-size: 0.9rem;
      margin: 12px 0 4px;
    }

    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 8px 10px;
      box-sizing: border-box;
      border-radius: 8px;
      border: 1px solid #d1d5db;
      font-size: 0.95rem;
    }

    input[type="text"]:focus,
    input[type="password"]:focus {
      outline: none;
      border-color: #2563eb;
      box-shadow: 0 0 0 1px #2563eb22;
    }

    button {
      cursor: pointer;
      border: none;
      border-radius: 9999px;
      padding: 8px 16px;
      font-size: 0.95rem;
      font-weight: 500;
      display: inline-flex;
      align-items: center;
      gap: 6px;
    }

    .btn-primary {
      background: #2563eb;
      color: white;
    }

    .btn-primary:active {
      transform: translateY(1px);
    }

    .btn-secondary {
      background: #e5e7eb;
      color: #111827;
    }

    .actions {
      margin-top: 16px;
      display: flex;
      justify-content: space-between;
      gap: 8px;
      flex-wrap: wrap;
    }

    .status {
      margin-top: 12px;
      font-size: 0.85rem;
      color: #6b7280;
    }

    .networks {
      margin-top: 20px;
      border-top: 1px solid #e5e7eb;
      padding-top: 16px;
    }

    .networks h2 {
      font-size: 1rem;
      margin: 0 0 8px;
    }

    .network-list {
      list-style: none;
      padding: 0;
      margin: 0;
      max-height: 180px;
      overflow-y: auto;
    }

    .network-item {
      border-radius: 8px;
      padding: 6px 8px;
      margin-bottom: 6px;
      border: 1px solid #e5e7eb;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.9rem;
    }

    .network-ssid {
      font-weight: 500;
    }

    .network-rssi {
      font-size: 0.8rem;
      color: #6b7280;
    }

    .small {
      font-size: 0.8rem;
      color: #9ca3af;
      margin-top: 4px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 WiFi konfiguracija</h1>
    <p class="subtitle">
      Spoji se na ovu stranicu, odaberi mrežu i upiši lozinku. Uređaj će se
      zatim restartati i pokušati spojiti na tvoj WiFi.
    </p>

    <form id="wifiForm" method="POST" action="/config">
      <label for="ssid">SSID (naziv mreže)</label>
      <input type="text" id="ssid" name="ssid" required>

      <label for="password">Lozinka</label>
      <input type="password" id="password" name="password" required>

      <div class="actions">
        <button type="submit" class="btn-primary">Spremi i restartaj</button>
        <button type="button" class="btn-secondary" id="scanBtn">Skeniraj mreže</button>
      </div>
    </form>

    <div class="status" id="statusText">
      Status: spremno.
    </div>

    <div class="networks">
      <h2>Dostupne mreže</h2>
      <ul class="network-list" id="networkList"></ul>
      <p class="small">Dodirni mrežu da automatski upiše SSID u polje iznad.</p>
    </div>
  </div>

  <script>
    const scanBtn = document.getElementById("scanBtn");
    const listEl = document.getElementById("networkList");
    const statusEl = document.getElementById("statusText");
    const ssidInput = document.getElementById("ssid");

    function setStatus(text) {
      statusEl.textContent = "Status: " + text;
      console.log(text);
    }

    scanBtn.addEventListener("click", () => {
      setStatus("skaniram WiFi mreže...");
      listEl.innerHTML = "";
      fetch("/scan")
        .then(r => {
          if (!r.ok) throw new Error("HTTP " + r.status);
          return r.json();
        })
        .then(data => {
          if (!Array.isArray(data)) {
            setStatus("neočekivan odgovor sa /scan");
            return;
          }

          if (data.length === 0) {
            setStatus("nema pronađenih mreža.");
            return;
          }

          data.sort((a, b) => b.rssi - a.rssi);
          setStatus("pronađeno " + data.length + " mreža.");

          data.forEach(ap => {
            const li = document.createElement("li");
            li.className = "network-item";

            const left = document.createElement("div");
            left.className = "network-ssid";
            left.textContent = ap.ssid || "(skriveni SSID)";

            const right = document.createElement("div");
            right.className = "network-rssi";
            right.textContent = ap.rssi + " dBm";

            li.appendChild(left);
            li.appendChild(right);

            li.addEventListener("click", () => {
              if (ap.ssid) {
                ssidInput.value = ap.ssid;
                setStatus("odabrana mreža: " + ap.ssid);
              }
            });

            listEl.appendChild(li);
          });
        })
        .catch(err => {
          console.error(err);
          setStatus("greška pri skeniranju: " + err.message);
        });
    });
  </script>
</body>
</html>
)HTML";

void setupWebServer() {
  Serial.println("Setting up web server...");

  if (!serverSetup) {
    // Glavna stranica - bilo koja metoda (GET, HEAD...)
    server.on("/", [](AsyncWebServerRequest *request) {
      Serial.print("Request: ");
      Serial.println(request->url());
      request->send_P(200, "text/html", htmlPage);
    });

    // /config – spremanje SSID / password, prihvaća bilo koju metodu (forma šalje POST)
    server.on("/config", [](AsyncWebServerRequest *request) {
      Serial.println("\n=== POST /config ===");

      if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password length: ");
        Serial.println(password.length());

        // Spremi u EEPROM
        saveWiFiConfig(ssid.c_str(), password.c_str());
        
        // Provjeri je li ispravno spremenj
        WiFiConfig testConfig;
        loadWiFiConfig(testConfig);
        Serial.print("Verification - SSID from EEPROM: ");
        Serial.println(testConfig.ssid);

        // Pošalji odgovor
        request->send(200, "text/plain", "OK! Rebooting...");

        // Čekaj malo prije resarta
        delay(2000);
        ESP.restart();
      } else {
        Serial.println("Missing SSID or password");
        request->send(400, "text/plain", "Missing params");
      }
    });

    // /scan – skeniranje mreža
    server.on("/scan", [](AsyncWebServerRequest *request) {
      Serial.println("Request /scan");

      int n = WiFi.scanNetworks(false, false);  // blokirajući scan

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

      Serial.print("Pronadeno mreza: ");
      Serial.println(n);

      request->send(200, "application/json", json);
      WiFi.scanDelete();
    });

    // 404 handler – ako je root ili /index.html, vrati glavnu stranicu
    server.onNotFound([](AsyncWebServerRequest *request) {
      Serial.print("Not found: ");
      Serial.println(request->url());

      if (request->url() == "/" || request->url() == "/index.html") {
        request->send_P(200, "text/html", htmlPage);
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
  // AsyncWebServer sam hendla konekcije, ovdje nije potrebno nista
}
