#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define MAX_LOG_SIZE 4096  // Max 4KB za logove

struct LogEntry {
  unsigned long timestamp;  // Unix timestamp ili millis
  char message[256];        // Log poruka
};

class Logger {
private:
  static const int EEPROM_LOG_START = 128;  // Početek logova u EEPROM (nakon WiFi config)
  static const int MAX_ENTRIES = 10;        // Max 10 log unosa

public:
  // Inicijalizacija logger-a
  static void init();

  // Logira grešku WiFi prekida
  static void logWiFiDisconnect();

  // Logira custom poruku
  static void logError(const char* message);

  // Očisti sve logove
  static void clearLogs();

  // Dohvati sve logove kao JSON string
  static String getLogsAsJSON();

  // Provjeri ima li nedoslanih logova
  static bool hasPendingLogs();

  // Broj logiranih grešaka
  static int getLogCount();
};

#endif
