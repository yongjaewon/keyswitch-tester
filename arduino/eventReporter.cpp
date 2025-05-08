//eventReporter.cpp
#include "eventReporter.h"
#include "config.h"
#include <Arduino.h>

void reportCycle(int station, unsigned long cycle, int failure, float keyswitchCurrent, float starterCurrent) {
    Serial.print("CYCLE:");
    Serial.print(station);
    Serial.print(":");
    Serial.print(cycle);
    Serial.print(":");
    Serial.print(failure);
    Serial.print(":");
    Serial.print(keyswitchCurrent);
    Serial.print(":");
    Serial.println(starterCurrent);
}

void reportEvent(const String& message) {
    Serial.print("EVENT:");
    Serial.println(message);
}

void requestLastSavedState() {
    Serial.println("REQUEST:STATE");
}