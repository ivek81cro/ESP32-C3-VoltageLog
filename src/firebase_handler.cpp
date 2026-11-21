#include "firebase_handler.h"
#include <ArduinoJson.h>
#include <string.h>
#include <WiFi.h>
#include <time.h>
#include "logger.h"

bool firebaseInitialized = false;
String idToken = "";
unsigned long tokenExpiryTime = 0;

// Record counter 1..20 (loop)
static int recordCounter = 0;
static bool timeSynced = false;

// Function to obtain ID Token via email/password authentication
bool getIdToken() {
  Serial.println(F("Pokušaj dobivanja ID Token-a..."));
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi nije spojen!"));
    return false;
  }

  // URL for signInWithPassword (email/password)
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + String(FIREBASE_API_KEY);

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // POST body with email/password (from firebase_config.h)
  String body = "{\"email\":\"" + String(FIREBASE_USER_EMAIL) + "\",\"password\":\"" + String(FIREBASE_USER_PASSWORD) + "\",\"returnSecureToken\": true}";
  
  Serial.println(F("Slanje POST zahtjeva (signInWithPassword)..."));
  int httpCode = http.POST(body);
  String response = http.getString();

  Serial.print(F("HTTP Code: "));
  Serial.println(httpCode);

  if (httpCode == 200) {
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      idToken = doc["idToken"].as<String>();
      if (idToken.isEmpty()) { http.end(); return false; }
      const char *uid = doc["localId"] | "";
      Serial.printf("[Firebase] Auth UID: %s\n", *uid ? uid : "unavailable");
      
      // Check expiration time (tokenExpires in seconds)
      int expiresIn = doc["expiresIn"] | 3600;  // default 1 hour
      tokenExpiryTime = millis() + (expiresIn * 1000);

      Serial.println(F("✓ ID Token uspješno dobiven!"));
      
      http.end();
      return true;
    } else {
      Serial.print(F("✗ Greška pri parsiranju JSON odgovora: "));
      Serial.println(error.c_str());
    }
  } else {
    Serial.print(F("✗ Greška pri dobivanju ID Token-a - HTTP kod: "));
    Serial.println(httpCode);
  }

  http.end();
  return false;
}

void initFirebase() {
  Serial.println(F("Inicijalizacija Firebase-a..."));
  
  // Attempt to get ID Token
  if (getIdToken()) {
    firebaseInitialized = true;
    Serial.println(F("✓ Firebase inicijaliziran!"));
    Serial.print(F("Database URL: "));
    Serial.println(FIREBASE_DATABASE_URL);
    configTime(0, 0, "pool.ntp.org", "time.google.com");

  } else {
    Serial.println(F("✗ Inicijalizacija Firebase-a neuspješna"));
    firebaseInitialized = false;
  }
}

// Ensure time is synced; call before using time. Non-blocking short attempt.
void ensureTimeSynced() {
  timeSynced = time(nullptr) >= 100000;
}

// Delete old entries (keep only the last 20)
void deleteOldEntries() {
  // Fetch all entries with orderBy key
  String url = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + ".json?orderBy=\"$key\"&limitToFirst=100&auth=" + idToken;
  
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.GET();
  String response = http.getString();
  
  if (httpCode == 200 && response != "null") {
    // Parse JSON response
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonObject>()) {
      JsonObject obj = doc.as<JsonObject>();
      
      // Count entries
      int count = 0;
      unsigned long oldestKey = ULONG_MAX;
      
      for (JsonPair p : obj) {
        count++;
        unsigned long keyNum = strtoul(p.key().c_str(), NULL, 10);
        if (keyNum < oldestKey) {
          oldestKey = keyNum;
        }
      }
      
      // If more than 20, delete the 5 oldest (avoid API limits)
      if (count > 20) {
        int deletedCount = 0;
        for (JsonPair p : obj) {
          if (deletedCount >= 5) break;  // Delete only 5 at a time
          
          unsigned long keyNum = strtoul(p.key().c_str(), NULL, 10);
          if (keyNum == oldestKey) {
            String deleteUrl = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + "/" + p.key().c_str() + ".json?auth=" + idToken;
            HTTPClient deleteHttp;
            deleteHttp.setConnectTimeout(5000);
            deleteHttp.setTimeout(5000);
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

  // Check if the token expired
  if (static_cast<int32_t>(millis() - tokenExpiryTime) >= 0) {
    Serial.println(F("ID Token istekao, ponovno se autentificiram..."));
    if (!getIdToken()) {
      return false;
    }
  }

  // Increment counter and loop 1..20
  recordCounter++;
  if (recordCounter > 20) {
    recordCounter = 1;
  }

  // Ensure time is synced (short attempt if not)
  ensureTimeSynced();

  // Create JSON object
  StaticJsonDocument<256> json;
  json["recordNumber"] = recordCounter;
  json["voltage"]   = voltage;
  json["rawValue"]  = rawValue;
  json["device"]    = "ESP32-C3-VoltageLog";

  // Add UTC timestamp (epoch) and ISO8601 string if available
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
    json["timestamp"] = 0; // marker that time is not synced
    json["utc_time"] = "unsynced";
  }

  String body;
  if (serializeJson(json, body) == 0) {
    Serial.println(F("✗ Greška pri serijalizaciji JSON-a"));
    return false;
  }

  // Use sequential number as child key (1-20 loop)
  String url = String(FIREBASE_DATABASE_URL) + FIREBASE_PATH + "/" + String(recordCounter) + ".json?auth=" + idToken;

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(body);
  if (httpCode == 200) {
    Serial.println(F("✓ Podaci uspješno poslani na Firebase!"));
    
    http.end();
    return true;
  } else {
    Serial.print(F("✗ Firebase greška - HTTP kod: "));
    Serial.println(httpCode);
    
    if (httpCode == 401) firebaseInitialized = false;

    
    http.end();
    return false;
  }
}

bool registerVoltageSensor(const String &deviceId) {
  if (!firebaseInitialized) return false;
  if (static_cast<int32_t>(millis() - tokenExpiryTime) >= 0 && !getIdToken()) return false;
  String url = String(FIREBASE_DATABASE_URL) + "/devices/" + deviceId + "/sensors/voltage.json?auth=" + idToken;
  HTTPClient http;
  http.setConnectTimeout(5000); http.setTimeout(5000);
  http.begin(url); http.addHeader("Content-Type", "application/json");
  int code = http.sendRequest("PATCH", "{\"type\":\"voltage\",\"unit\":\"V\"}");
  http.end();
  if (code != 200) Serial.printf("[Schema] Sensor registration failed: HTTP %d\n", code);
  if (code == 401) firebaseInitialized = false;
  return code == 200;
}

bool sendSchemaVoltage(const String &deviceId, const String &readingId, float voltage, int rawValue, uint32_t measuredAtSeconds) {
  if (!firebaseInitialized || measuredAtSeconds == 0) return false;
  if (static_cast<int32_t>(millis() - tokenExpiryTime) >= 0 && !getIdToken()) return false;
  StaticJsonDocument<256> json;
  json["timestamp"] = static_cast<double>(measuredAtSeconds) * 1000.0;
  json["value"] = voltage;
  json["rawValue"] = rawValue;
  String body; serializeJson(json, body);
  String url = String(FIREBASE_DATABASE_URL) + "/readings/" + deviceId + "/voltage/" + readingId + ".json?auth=" + idToken;
  HTTPClient http;
  http.setConnectTimeout(5000); http.setTimeout(5000);
  http.begin(url); http.addHeader("Content-Type", "application/json");
  int code = http.PUT(body);
  if (code != 200) {
    String response = http.getString();
    StaticJsonDocument<256> error;
    if (!deserializeJson(error, response)) {
      const char *reason = error["error"] | "";
      if (strcmp(reason, "Permission denied") == 0 || strcmp(reason, "Permission denied.") == 0)
        Serial.println("[Schema] Firebase: permission denied");
      else if (strcmp(reason, "Auth token is expired") == 0)
        Serial.println("[Schema] Firebase: auth token expired");
      else Serial.println("[Schema] Firebase: unrecognized error");
    }
  }
  http.end();
  if (code != 200) Serial.printf("[Schema] Reading write failed: HTTP %d\n", code);
  else Serial.println("[Schema] Voltage reading saved");
  if (code == 401) firebaseInitialized = false;
  return code == 200;
}

bool checkFirebaseConnection() {
  return firebaseInitialized && !idToken.isEmpty();
}

bool sendLogsToFirebase() {
  if (!firebaseInitialized) {
    Serial.println(F("Firebase nije inicijaliziran za slanje logova"));
    return false;
  }

  // Provjeri ima li logova za slanje
  if (!Logger::hasPendingLogs()) {
    Serial.println(F("Nema logova za slanje"));
    return true;
  }

  // Check if token expired
  if (static_cast<int32_t>(millis() - tokenExpiryTime) >= 0) {
    Serial.println(F("ID Token istekao, ponovno se autentificiram..."));
    if (!getIdToken()) {
      return false;
    }
  }

  // Dohvati logove kao JSON
  String logsJSON = Logger::getLogsAsJSON();
  
  Serial.println(F("Slanje logova na Firebase..."));

  // URL za slanje logova (u drugoj bazi ili path-u)
  String url = String(FIREBASE_DATABASE_URL) + "/logs.json?auth=" + idToken;

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(logsJSON);
  if (httpCode == 200) {
    Serial.println(F("✓ Logovi uspješno poslani na Firebase!"));
    Logger::clearLogs();  // Obriši logove nakon uspješnog slanja
    http.end();
    return true;
  } else {
    Serial.print(F("✗ Greška pri slanju logova - HTTP kod: "));
    Serial.println(httpCode);
    
    // Ako je 401, pokušaj ponovno autentifikaciju
    if (httpCode == 401) {
      Serial.println(F("Neautoriziran pristup, ponovno autentifikacija..."));
      if (getIdToken()) {
        // Pokušaj ponovno
        String newUrl = String(FIREBASE_DATABASE_URL) + "/logs.json?auth=" + idToken;
        HTTPClient retryHttp;
        retryHttp.setConnectTimeout(5000);
        retryHttp.setTimeout(5000);
        retryHttp.begin(newUrl);
        retryHttp.addHeader("Content-Type", "application/json");
        int retryCode = retryHttp.POST(logsJSON);
        
        if (retryCode == 200) {
          Serial.println(F("✓ Slanje logova uspješno nakon ponovne autentifikacije!"));
          Logger::clearLogs();
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
