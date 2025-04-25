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

void reportComplete(int station) {
    Serial.print("COMPLETE:");
    Serial.println(station);
}

void reportFailure(int station, int failCount) {
    Serial.print("FAIL:");
    Serial.println(station);
    Serial.print("FAIL_COUNT:");
    Serial.print(station);
    Serial.print(":");
    Serial.println(failCount);
}

void reportStatus() {
    Serial.print("STATUS:");
    Serial.println(masterEnabled ? "RUNNING" : "IDLE");
    Serial.print("STATION:");
    Serial.println(activeStationIndex);
}