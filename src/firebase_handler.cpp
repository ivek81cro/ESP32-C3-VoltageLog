#include "firebase_handler.h"
#include <ArduinoJson.h>
#include <string.h>
#include <WiFi.h>
#include <time.h>

bool firebaseInitialized = false;
String idToken = "";
unsigned long tokenExpiryTime = 0;

// Brojač zapisa 1..20 (loop)
static int recordCounter = 0;
static bool timeSynced = false;

// Funkcija za dobivanje ID Token-a putem email/password autentifikacije
bool getIdToken() {
  Serial.println(F("Pokušaj dobivanja ID Token-a..."));
  
  // Provjera je li WiFi spojen
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi nije spojen!"));
    return false;
  }

  // URL za signInWithPassword (email/password)
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + String(FIREBASE_API_KEY);
  Serial.print(F("URL: "));
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // POST body sa email/lozinka (iz firebase_config.h)
  String body = "{\"email\":\"" + String(FIREBASE_USER_EMAIL) + "\",\"password\":\"" + String(FIREBASE_USER_PASSWORD) + "\",\"returnSecureToken\": true}";
  
  Serial.println(F("Slanje POST zahtjeva (signInWithPassword)..."));
  int httpCode = http.POST(body);
  String response = http.getString();

  Serial.print(F("HTTP Code: "));
  Serial.println(httpCode);
  Serial.print(F("Response: "));
  Serial.println(response);

  if (httpCode == 200) {
    // Parsiranje JSON odgovora
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      idToken = doc["idToken"].as<String>();
      
      // Provjera expiration time (tokenExpires je u sekundama)
      int expiresIn = doc["expiresIn"] | 3600;  // default 1 sat
      tokenExpiryTime = millis() + (expiresIn * 1000);

      Serial.println(F("✓ ID Token uspješno dobiven!"));
      Serial.print(F("Token (first 20 chars): "));
      Serial.println(idToken.substring(0, 20));
      
      http.end();
      return true;
    } else {
      Serial.print(F("✗ Greška pri parsiranju JSON odgovora: "));
      Serial.println(error.c_str());
    }
  } else {
    Serial.print(F("✗ Greška pri dobivanju ID Token-a - HTTP kod: "));
    Serial.println(httpCode);
    Serial.println(response);
  }

  http.end();
  return false;
}

void initFirebase() {
  Serial.println(F("Inicijalizacija Firebase-a..."));
  
  // Pokušaj dobiti ID Token
  if (getIdToken()) {
    firebaseInitialized = true;
    Serial.println(F("✓ Firebase inicijaliziran!"));
    Serial.print(F("Database URL: "));
    Serial.println(FIREBASE_DATABASE_URL);
    // Pokušaj sinkronizirati vrijeme s NTP-om (UTC)
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    // pokušaj sinkronizacije vremena (kratki timeout)
    timeSynced = false;
    time_t now = time(nullptr);
    unsigned long start = millis();
    while ((now < 100000) && (millis() - start < 10000)) {
      delay(500);
      now = time(nullptr);
    }
    if (now >= 100000) {
      timeSynced = true;
      Serial.println(F("✓ Vrijeme sinkronizirano (UTC)."));
    } else {
      Serial.println(F("✗ Neuspjela sinkronizacija vremena (NTP) - pokušat će se kasnije."));
    }
  } else {
    Serial.println(F("✗ Inicijalizacija Firebase-a neuspješna"));
    firebaseInitialized = false;
  }
}

// Ensure time is synced; call before using time. Non-blocking short attempt.
void ensureTimeSynced() {
  if (timeSynced) return;
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  time_t now = time(nullptr);
  unsigned long start = millis();
  while ((now < 100000) && (millis() - start < 5000)) {
    delay(300);
    now = time(nullptr);
  }
  if (now >= 100000) {
    timeSynced = true;
    Serial.println(F("✓ Vrijeme sinkronizirano (UTC) (on-demand)."));
  }
}

// Funkcija za brisanje starih unosa (čuva samo zadnjih 20)
void deleteOldEntries() {
  // Dohvati sve unose sa orderBy key
  String url = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + ".json?orderBy=\"$key\"&limitToFirst=100&auth=" + idToken;
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.GET();
  String response = http.getString();
  
  if (httpCode == 200 && response != "null") {
    // Parsiranje JSON odgovora
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonObject>()) {
      JsonObject obj = doc.as<JsonObject>();
      
      // Broji unose
      int count = 0;
      unsigned long oldestKey = ULONG_MAX;
      
      for (JsonPair p : obj) {
        count++;
        unsigned long keyNum = strtoul(p.key().c_str(), NULL, 10);
        if (keyNum < oldestKey) {
          oldestKey = keyNum;
        }
      }
      
      // Ako ima više od 20, obriši 5 najstarijih (da ne kršiš limit na API)
      if (count > 20) {
        int deletedCount = 0;
        for (JsonPair p : obj) {
          if (deletedCount >= 5) break;  // Obriši samo 5 odjednom
          
          unsigned long keyNum = strtoul(p.key().c_str(), NULL, 10);
          if (keyNum == oldestKey) {
            String deleteUrl = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + "/" + p.key().c_str() + ".json?auth=" + idToken;
            HTTPClient deleteHttp;
            deleteHttp.begin(deleteUrl);
            int delCode = deleteHttp.sendRequest("DELETE");
            deleteHttp.end();
            
            if (delCode == 200) {
              deletedCount++;
              Serial.print(F("✓ Obrisan unos: "));
              Serial.println(p.key().c_str());
            }
          }
        }
      }
    }
  }
  
  http.end();
}

bool sendVoltageToFirebase(float voltage, int rawValue) {
  if (!firebaseInitialized) {
    Serial.println(F("Firebase nije inicijaliziran"));
    return false;
  }

  // Provjera je li token istekao
  if (millis() > tokenExpiryTime) {
    Serial.println(F("ID Token istekao, ponovno se autentificiram..."));
    if (!getIdToken()) {
      return false;
    }
  }

  // Povećaj brojač i vrti ga 1..20
  recordCounter++;
  if (recordCounter > 20) {
    recordCounter = 1;
  }

  // Osiguraj da je vrijeme sinkronizirano (pokuša kratko ako nije)
  ensureTimeSynced();

  // Kreiranje JSON objekta
  StaticJsonDocument<256> json;
  json["recordNumber"] = recordCounter;
  json["voltage"]   = voltage;
  json["rawValue"]  = rawValue;
  json["device"]    = "ESP32-C3-VoltageLog";

  // Dodaj UTC timestamp (epoch) i ISO8601 string ako je dostupno
  time_t now = time(nullptr);
  if (now >= 100000) {
    json["timestamp"] = (unsigned long)now;
    struct tm *tminfo = gmtime(&now);
    char timebuf[32] = {0};
    if (tminfo != NULL) {
      strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", tminfo);
      json["utc_time"] = timebuf;
    } else {
      json["utc_time"] = "";
    }
  } else {
    json["timestamp"] = 0; // oznaka da nije sinkronizirano
    json["utc_time"] = "unsynced";
  }

  String body;
  if (serializeJson(json, body) == 0) {
    Serial.println(F("✗ Greška pri serijalizaciji JSON-a"));
    return false;
  }

  // Koristi redni broj kao child key (1-20 u loop-u)
  String url = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + "/" + String(recordCounter) + ".json?auth=" + idToken;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(body);
  String response = http.getString();

  if (httpCode == 200) {
    Serial.println(F("✓ Podaci uspješno poslani na Firebase!"));
    
    // Sada obriši stare unose - čuva samo zadnjih 20 (svaki 10. unos)
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup > 30000) {  // Čisti svakih 30 sekundi
      deleteOldEntries();
      lastCleanup = millis();
    }
    
    http.end();
    return true;
  } else {
    Serial.print(F("✗ Firebase greška - HTTP kod: "));
    Serial.println(httpCode);
    Serial.print(F("Odgovor: "));
    Serial.println(response);
    
    // Ako je 401, pokušaj ponovno dobiti token
    if (httpCode == 401) {
      Serial.println(F("Neautoriziran pristup, ponovno autentifikacija..."));
      if (getIdToken()) {
        // Ponovi zahtjev sa novim tokenom
        String newUrl = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + "/" + String(recordCounter) + ".json?auth=" + idToken;
        HTTPClient retryHttp;
        retryHttp.begin(newUrl);
        retryHttp.addHeader("Content-Type", "application/json");
        int retryCode = retryHttp.PUT(body);
        
        if (retryCode == 200) {
          Serial.println(F("✓ Retry uspješan!"));
          retryHttp.end();
          return true;
        }
        retryHttp.end();
      }
    }
    
    http.end();
    return false;
  }
}

bool checkFirebaseConnection() {
  return firebaseInitialized && !idToken.isEmpty();
}
