#include "logger.h"
#include <EEPROM.h>
#include <time.h>
#include <ArduinoJson.h>

// Struktura za pohranu broja logova i flaga
struct LoggerHeader {
  int entryCount;
  bool hasUnsent;
};

// Memorijska mapa:
// EEPROM 128-131: LoggerHeader (4 bytes)
// EEPROM 132-3327: Log unosi (10 * 320 bytes)

const int HEADER_ADDR = 128;
const int LOGS_START_ADDR = 132;
const int ENTRY_SIZE = 320;  // sizeof(unsigned long) + 256 char
const int MAX_ENTRIES = 10;

void Logger::init() {
  EEPROM.begin(4096);
  
  // Provjeri je li header inicijaliziran
  LoggerHeader header;
  EEPROM.get(HEADER_ADDR, header);
  
  // Ako je prvi put, inicijalizira
  if (header.entryCount < 0 || header.entryCount > MAX_ENTRIES) {
    header.entryCount = 0;
    header.hasUnsent = false;
    EEPROM.put(HEADER_ADDR, header);
    EEPROM.commit();
    Serial.println("[Logger] Inicijalizacija logera");
  } else if (header.entryCount > 0) {
    Serial.print("[Logger] Pronađeno ");
    Serial.print(header.entryCount);
    Serial.println(" nedoslanih logova");
  }
}

void Logger::logWiFiDisconnect() {
  logError("WiFi disconnect");
}

void Logger::logError(const char* message) {
  LoggerHeader header;
  EEPROM.get(HEADER_ADDR, header);

  // Ne dodaj ako je pun
  if (header.entryCount >= MAX_ENTRIES) {
    Serial.println("[Logger] Log memorija puna!");
    return;
  }

  // Kreiraj novi unos
  LogEntry entry;
  entry.timestamp = time(nullptr);
  if (entry.timestamp < 100000) {
    entry.timestamp = millis() / 1000;  // Fallback na millis ako vrijeme nije sinkronizirano
  }
  
  strncpy(entry.message, message, 255);
  entry.message[255] = '\0';

  // Spremi u EEPROM
  int addr = LOGS_START_ADDR + (header.entryCount * ENTRY_SIZE);
  EEPROM.put(addr, entry);
  
  // Ažurira header
  header.entryCount++;
  header.hasUnsent = true;
  EEPROM.put(HEADER_ADDR, header);
  EEPROM.commit();

  Serial.print("[Logger] Logirana greška: ");
  Serial.println(message);
}

void Logger::clearLogs() {
  LoggerHeader header;
  header.entryCount = 0;
  header.hasUnsent = false;
  
  EEPROM.put(HEADER_ADDR, header);
  EEPROM.commit();
  
  Serial.println("[Logger] Svi logovi obrisani");
}

String Logger::getLogsAsJSON() {
  LoggerHeader header;
  EEPROM.get(HEADER_ADDR, header);

  DynamicJsonDocument doc(4096);
  JsonArray logsArray = doc.createNestedArray("logs");

  // Pročitaj sve logove
  for (int i = 0; i < header.entryCount; i++) {
    int addr = LOGS_START_ADDR + (i * ENTRY_SIZE);
    LogEntry entry;
    EEPROM.get(addr, entry);

    JsonObject logObj = logsArray.createNestedObject();
    logObj["timestamp"] = entry.timestamp;
    logObj["message"] = entry.message;
  }

  doc["count"] = header.entryCount;
  doc["timestamp"] = time(nullptr);

  String jsonStr;
  serializeJson(doc, jsonStr);
  return jsonStr;
}

bool Logger::hasPendingLogs() {
  LoggerHeader header;
  EEPROM.get(HEADER_ADDR, header);
  return header.hasUnsent && header.entryCount > 0;
}

int Logger::getLogCount() {
  LoggerHeader header;
  EEPROM.get(HEADER_ADDR, header);
  return header.entryCount;
}
