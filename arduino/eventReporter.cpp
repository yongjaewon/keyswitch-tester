//eventReporter.cpp
#include "eventReporter.h"
#include "config.h"
#include "stateMachine.h"
#include "stationManager.h"
#include <Arduino.h>

void reportCycle(int station, boolean enabled, unsigned long cycles, int failures, float keyswitchCurrent, float starterCurrent) {
    Serial.print("CYCLE:");
    Serial.print(station);
    Serial.print(":");
    Serial.print(enabled);
    Serial.print(":");
    Serial.print(cycles);
    Serial.print(":");
    Serial.print(failures);
    Serial.print(":");
    Serial.print(keyswitchCurrent);
    Serial.print(":");
    Serial.println(starterCurrent);
}

void reportEvent(const String& message) {
    Serial.print("EVENT:");
    Serial.println(message);
}

void reportSystemState() {
    String stateData = "SYSTEM_STATE:";
    stateData += String(isSystemRunning() ? 1 : 0);
    Serial.println(stateData);
}

void reportStationState(int station) {
    String stateData = "STATION:";
    stateData += String(station) + ":" + 
                 String(isStationEnabled(station) ? 1 : 0) + ":" + 
                 String(getStationCycles(station)) + ":" + 
                 String(getStationFailures(station));
    Serial.println(stateData);
}

void reportStationStates() {
    for (int i = 0; i < STATION_COUNT; i++) {
        reportStationState(i);
    }
}