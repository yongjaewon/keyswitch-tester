//eventReporter.cpp
#include "eventReporter.h"
#include "config.h"
#include <Arduino.h>

void reportCurrent(int station, float current) {
    Serial.print("CURRENT:");
    Serial.print(station);
    Serial.print(":");
    Serial.println(current);
}

void reportEvent(const String& message) {
    Serial.print("EVENT:");
    Serial.println(message);
}