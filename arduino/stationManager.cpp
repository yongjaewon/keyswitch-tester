//stationManager.cpp
#include "stationManager.h"
#include "config.h"
#include "eventReporter.h"

static bool stationEnabled[STATION_COUNT] = {true, true, true, true};
static unsigned long cycleCount[STATION_COUNT] = {118875, 113758, 0, 0};
static int failureCount[STATION_COUNT] = {0, 0, 0, 0};

// Station management functions
int getEnabledStationCount() {
    int count = 0;
    for (int i = 0; i < STATION_COUNT; i++) {
        if (stationEnabled[i]) count++;
    }
    return count;
}

bool enableStation(int station) {
    if (station < 0 || station >= STATION_COUNT) {
        reportEvent("Invalid station index: " + String(station));
        return false;
    }
    if (stationEnabled[station]) {
        reportEvent("Station " + String(station) + " is already enabled");
        return false;
    }
    stationEnabled[station] = true;
    reportStationState(station);
    reportEvent("Station " + String(station) + " enabled");
    return true;
}

bool disableStation(int station) {
    if (station < 0 || station >= STATION_COUNT) {
        reportEvent("Invalid station index: " + String(station));
        return false;
    }
    if (!stationEnabled[station]) {
        reportEvent("Station " + String(station) + " is already disabled");
        return false;
    }
    stationEnabled[station] = false;
    reportStationState(station);
    reportEvent("Station " + String(station) + " disabled");
    return true;
}

// Counter management functions
void updateStationCounters(int station, unsigned long cycles, int failures) {
    cycleCount[station] = cycles;
    failureCount[station] = failures;
    reportStationState(station);
}

void incrementStationCycles(int station) {
    cycleCount[station]++;
    reportStationState(station);
}

void incrementStationFailures(int station) {
    failureCount[station]++;
    reportStationState(station);
}

void resetStationCycleCount(int station) {
    if (station >= 0 && station < STATION_COUNT) {
        cycleCount[station] = 0;
        reportStationState(station);
        reportEvent("Station " + String(station) + " cycle count reset to 0");
    }
}

void resetStationFailureCount(int station) {
    if (station >= 0 && station < STATION_COUNT) {
        failureCount[station] = 0;
        reportStationState(station);
        reportEvent("Station " + String(station) + " failure count reset to 0");
    }
}

// State accessor functions
unsigned long getStationCycles(int station) {
    return cycleCount[station];
}

int getStationFailures(int station) {
    return failureCount[station];
}

bool isStationEnabled(int station) {
    return stationEnabled[station];
} 