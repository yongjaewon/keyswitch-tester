//eventReporter.h
#pragma once
#include <Arduino.h>

void reportCycle(int station, unsigned long cycle, int failure, float keyswitchCurrent, float starterCurrent);
void reportEvent(const String& message);
void requestLastSavedState();