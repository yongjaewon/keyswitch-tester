//eventReporter.h
#pragma once
#include <Arduino.h>

void reportCurrent(int station, float current);
void reportEvent(const String& message);