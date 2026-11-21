#pragma once
#include <Arduino.h>
struct Reading
{
    int raw;
    float voltage;
};
void initMeasurement();
Reading measureVoltage();
