#pragma once
#include <Arduino.h>
#include "config.h"

struct DeviceStatus
{
  float voltage = 0;
  int raw = 0;
  uint32_t lastRead = 0;
  uint32_t measuredAtSeconds = 0;
  uint32_t generation = 0;
  uint32_t lastAttempt = 0;
  uint32_t lastSuccess = 0;
  uint32_t nextAttemptIn = 0;
  bool hasReading = false;
  bool hasAttempt = false;
  bool hasSuccess = false;
  bool wifi = false;
  bool token = false;
  bool transportBusy = false;
};
DeviceStatus getStatus();
void updateMeasurement(float voltage, int raw, uint32_t now, uint32_t measuredAtSeconds);
void updateWifi(bool connected);
void updateTransport(const DeviceStatus &status);
bool queueConfig(const WiFiConfig &config);
bool takeConfig(WiFiConfig &config);
