#include "measurement.h"
static constexpr int pin = 4;
void initMeasurement()
{
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(pin, INPUT);
}
Reading measureVoltage()
{
    int raw = analogRead(pin);
    return {raw, raw * (3.3f / 4095.0f) * 0.91f * 5.0f};
}
