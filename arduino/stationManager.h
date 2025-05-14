//stationManager.h
#pragma once
#include <Arduino.h>

// Station management functions
int getEnabledStationCount();
bool enableStation(int station);
bool disableStation(int station);

// Counter management functions
void updateStationCounters(int station, unsigned long cycles, int failures);
void incrementStationCycles(int station);
void incrementStationFailures(int station);
void resetStationCycleCount(int station);
void resetStationFailureCount(int station);

// State accessor functions
unsigned long getStationCycles(int station);
int getStationFailures(int station);
bool isStationEnabled(int station); 