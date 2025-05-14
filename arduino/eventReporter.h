//eventReporter.h
#pragma once
#include <Arduino.h>

void reportCycle(int station, boolean enabled, unsigned long cycles, int failures, float keyswitchCurrent, float starterCurrent);
void reportEvent(const String& message);
void reportSystemState();
void reportStationState(int station);
void reportStationStates();