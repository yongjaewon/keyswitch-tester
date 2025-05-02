//eventReporter.h
#pragma once

#include <Arduino.h>

void reportCurrent(int station, float current);
void reportComplete(int station);
void reportFailure(int station, int failCount);
void reportStatus();
void reportEvent(const String& message);