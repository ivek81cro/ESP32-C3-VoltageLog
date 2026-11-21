#include "state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static DeviceStatus state;
static WiFiConfig pending;
static bool hasPending = false;
DeviceStatus getStatus()
{
  portENTER_CRITICAL(&lock);
  DeviceStatus copy = state;
  portEXIT_CRITICAL(&lock);
  return copy;
}
void updateMeasurement(float voltage, int raw, uint32_t now, uint32_t measuredAtSeconds)
{
  portENTER_CRITICAL(&lock);
  state.voltage = voltage;
  state.raw = raw;
  state.lastRead = now;
  state.measuredAtSeconds = measuredAtSeconds;
  ++state.generation;
  state.hasReading = true;
  portEXIT_CRITICAL(&lock);
}
void updateWifi(bool connected)
{
  portENTER_CRITICAL(&lock);
  state.wifi = connected;
  portEXIT_CRITICAL(&lock);
}
void updateTransport(const DeviceStatus &value)
{
  portENTER_CRITICAL(&lock);
  state.lastAttempt = value.lastAttempt;
  state.lastSuccess = value.lastSuccess;
  state.nextAttemptIn = value.nextAttemptIn;
  state.hasAttempt = value.hasAttempt;
  state.hasSuccess = value.hasSuccess;
  state.token = value.token;
  state.transportBusy = value.transportBusy;
  portEXIT_CRITICAL(&lock);
}
bool queueConfig(const WiFiConfig &value)
{
  portENTER_CRITICAL(&lock);
  bool accepted = !hasPending;
  if (accepted)
  {
    pending = value;
    hasPending = true;
  }
  portEXIT_CRITICAL(&lock);
  return accepted;
}
bool takeConfig(WiFiConfig &value)
{
  portENTER_CRITICAL(&lock);
  bool available = hasPending;
  if (available)
  {
    value = pending;
    hasPending = false;
  }
  portEXIT_CRITICAL(&lock);
  return available;
}
