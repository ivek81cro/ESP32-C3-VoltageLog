#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE   512
#define SSID_MAX_LEN  32
#define PASS_MAX_LEN  64

struct WiFiConfig {
  char ssid[SSID_MAX_LEN + 1];
  char password[PASS_MAX_LEN + 1];
};

// Sve spremamo kao jedan struct od adrese 0
const int WIFI_CONFIG_ADDR = 0;

inline void initEEPROM() {
  static bool initialized = false;
  if (!initialized) {
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
  }
}

inline void saveWiFiConfig(const char* ssid, const char* password) {
  initEEPROM();

  WiFiConfig config{};
  // Sigurna kopija SSID-a
  strncpy(config.ssid, ssid, SSID_MAX_LEN);
  config.ssid[SSID_MAX_LEN] = '\0';

  // Sigurna kopija passworda
  strncpy(config.password, password, PASS_MAX_LEN);
  config.password[PASS_MAX_LEN] = '\0';

  EEPROM.put(WIFI_CONFIG_ADDR, config);
  EEPROM.commit();
}

inline void loadWiFiConfig(WiFiConfig& config) {
  initEEPROM();
  EEPROM.get(WIFI_CONFIG_ADDR, config);

  // Osiguraj null-terminator
  config.ssid[SSID_MAX_LEN] = '\0';
  config.password[PASS_MAX_LEN] = '\0';
}

// Mala helper funkcija da ne ovisimo o strnlen
inline size_t safeStrLen(const char* s, size_t maxLen) {
  size_t len = 0;
  while (len < maxLen && s[len] != '\0') {
    ++len;
  }
  return len;
}

inline bool hasValidWiFiConfig() {
  WiFiConfig config{};
  loadWiFiConfig(config);

  // Ako je EEPROM prazan, prvi bajt će biti 0xFF ili '\0'
  uint8_t first = static_cast<uint8_t>(config.ssid[0]);
  if (first == 0xFF || config.ssid[0] == '\0') {
    return false;
  }

  size_t ssidLen = safeStrLen(config.ssid, SSID_MAX_LEN);
  size_t passLen = safeStrLen(config.password, PASS_MAX_LEN);

  return (ssidLen > 0 && passLen > 0 &&
          ssidLen <= SSID_MAX_LEN && passLen <= PASS_MAX_LEN);
}

// Po potrebi – ručno brisanje WiFi postavki
inline void clearWiFiConfig() {
  WiFiConfig empty{};
  EEPROM.put(WIFI_CONFIG_ADDR, empty);
  EEPROM.commit();
}

#endif
