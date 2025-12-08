#include "firebase_handler.h"
#include <ArduinoJson.h>
#include <string.h>

bool firebaseInitialized = false;
String idToken = "";
unsigned long tokenExpiryTime = 0;
static int recordCounter = 0;  // Brojač zapisa od 1-20

// Funkcija za dobivanje ID Token-a putem anonimne autentifikacije
bool getIdToken() {
  Serial.println(F("Pokušaj dobivanja ID Token-a..."));
  
  // Provjera je li WiFi spojen
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi nije spojen!"));
    return false;
  }

  // URL za anonimnu autentifikaciju
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + String(FIREBASE_API_KEY);
  Serial.print(F("URL: "));
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // POST zahtjev za anonimnu registraciju
  String body = "{\"returnSecureToken\": true}";
  
  Serial.println(F("Slanje POST zahtjeva..."));
  int httpCode = http.POST(body);
  String response = http.getString();

  Serial.print(F("HTTP Code: "));
  Serial.println(httpCode);
  Serial.print(F("Response: "));
  Serial.println(response);

  if (httpCode == 200) {
    // Parsiranje JSON odgovora - koristi DynamicJsonDocument zaFlexibilnost
    DynamicJsonDocument doc(2048);  // Dinamički dokument sa max 2048 bajta
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
  } else {
    Serial.println(F("✗ Inicijalizacija Firebase-a neuspješna"));
    firebaseInitialized = false;
  }
}

// Funkcija za brisanje starih unosa (čuva samo zadnjih 20)
void deleteOldEntries() {
  // Dohvati sve unose sa orderBy timestamp
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

  // Kreiranje JSON objekta
  StaticJsonDocument<256> json;
  json["timestamp"] = millis();
  json["voltage"]   = voltage;
  json["rawValue"]  = rawValue;
  json["device"]    = "ESP32-C3-VoltageLog";
  json["recordNumber"] = recordCounter;

  String body;
  if (serializeJson(json, body) == 0) {
    Serial.println(F("✗ Greška pri serijalizaciji JSON-a"));
    return false;
  }

  // Koristi redni broj kao child key (1-20 u loop-u)
  recordCounter++;
  if (recordCounter > 20) {
    recordCounter = 1;
  }
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
