//commandParser.h
#pragma once
#include <Arduino.h>

void handleCommands();
void processStateData(const String& stateData);
void resetStationCycles(int station);
void resetStationFailures(int station);