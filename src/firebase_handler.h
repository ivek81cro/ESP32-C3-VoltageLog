#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "firebase_config.h"

extern bool firebaseInitialized;

void initFirebase();
bool sendVoltageToFirebase(float voltage, int rawValue);
bool checkFirebaseConnection();

#endif
