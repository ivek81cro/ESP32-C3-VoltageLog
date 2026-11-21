#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "firebase_config.h"

extern bool firebaseInitialized;

void initFirebase();
bool sendVoltageToFirebase(float voltage, int rawValue);
bool registerVoltageSensor(const String &deviceId);
bool sendSchemaVoltage(const String &deviceId, const String &readingId, float voltage, int rawValue, uint32_t measuredAtSeconds);
bool checkFirebaseConnection();
bool sendLogsToFirebase();  // Nova funkcija za slanje logova

#endif
