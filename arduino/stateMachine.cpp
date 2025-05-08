//stateMachine.cpp
#include "stateMachine.h"
#include "config.h"
#include "eventReporter.h"
#include "currentMeasurer.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

//This namespace is required to use Dynamixel control table item names
using namespace ControlTableItem;

static CycleState currentState = CycleState::STOPPED;
static bool stationEnabled[STATION_COUNT] = {true, true, true, true};
static unsigned long cycleCount[STATION_COUNT] = {57924, 49105, 70197, 44023};
static int failureCount[STATION_COUNT] = {0, 0, 0, 0};
static unsigned long stateStartTime = 0;
static int activeStationIndex = 0;

int getEnabledStationCount() {
    int count = 0;
    for (int i = 0; i < STATION_COUNT; i++) {
        if (stationEnabled[i]) {
            count++;
        }
    }
    return count;
}

bool enableStation(int station) {
    if (station < 0 || station >= STATION_COUNT) {
        reportEvent("Invalid station index: " + String(station));
        return false;
    } else if (stationEnabled[station]) {
        reportEvent("Station " + String(station) + " is already enabled");
        return false;
    }
    stationEnabled[station] = true;
    reportEvent("Station " + String(station) + " enabled");
    return true;
}

bool disableStation(int station) {
    if (station < 0 || station >= STATION_COUNT) {
        reportEvent("Invalid station index: " + String(station));
        return false;
    } else if (!stationEnabled[station]) {
        reportEvent("Station " + String(station) + " is already disabled");
        return false;
    }
    stationEnabled[station] = false;
    reportEvent("Station " + String(station) + " disabled");
    return true;
}

bool startSystem() {
    if (currentState == CycleState::STOPPED || currentState == CycleState::EMERGENCY_STOPPED) {
        if (digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED) {
            reportEvent("Unable to start system. Emergency stop condition detected.");
            return false;
        }
        currentState = CycleState::READY;
        reportEvent("System started.");
        return true;
    }
    reportEvent("System is already running.");
    return false;
}

bool stopSystem() {
    if (currentState == CycleState::STOPPED || currentState == CycleState::EMERGENCY_STOPPED) {
        reportEvent("System is already stopped.");
        return false;
    }
    currentState = CycleState::STOPPED;
    reportEvent("System stopped.");
    return true;
}

void checkEmergencyStop() {
    bool estopPressed = digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED;

    if (estopPressed && currentState != CycleState::EMERGENCY_STOPPED) {
        for (int i = 0; i < STATION_COUNT; i++) {
            servoBus.setGoalPosition(i, SERVO_ANGLE_HOME, UNIT_DEGREE);
        }
        currentState = CycleState::EMERGENCY_STOPPED;
        reportEvent("Emergency stop detected");
    } else if (!estopPressed && currentState == CycleState::EMERGENCY_STOPPED) {
        currentState = CycleState::STOPPED;
        reportEvent("Emergency stop condition cleared");
    }
}

void handleStates() {
    unsigned long currentTime = millis();
    
    checkEmergencyStop();
    
    switch (currentState) {
        case CycleState::READY:
            if (stationEnabled[activeStationIndex]) {
                stateStartTime = currentTime;
                currentState = CycleState::ACTUATING_START;
            } else {
                activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            }
            break;

        case CycleState::ACTUATING_START:            
            trackPeakCurrents();
            servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_START, UNIT_DEGREE);
            if (currentTime - stateStartTime > ROTATE_TO_START_DURATION_MS) {
                currentState = CycleState::ACTUATING_HOME;
            }
            break;

        case CycleState::ACTUATING_HOME:
            trackPeakCurrents();
            servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_HOME, UNIT_DEGREE);
            if (currentTime - stateStartTime > ROTATE_TO_HOME_DURATION_MS) {
                currentState = CycleState::PROCESSING;
            }
            break;

        case CycleState::PROCESSING: {
            cycleCount[activeStationIndex]++;
            float keyswitchCurrent = getPeakKeyswitchCurrent();
            float starterCurrent = getPeakStarterCurrent();
            if (keyswitchCurrent < KEYSWITCH_CURRENT_THRESHOLD || starterCurrent < STARTER_CURRENT_THRESHOLD) {
                failureCount[activeStationIndex]++;
            }
            reportCycle(activeStationIndex, cycleCount[activeStationIndex], failureCount[activeStationIndex], keyswitchCurrent, starterCurrent);
            
            activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            currentState = CycleState::WAITING;
            break;
        }

        case CycleState::WAITING: {
            int enabledStationCount = getEnabledStationCount();
            int cyclePeriodMs = (enabledStationCount == 0) ? 0 : (60000 / (CYCLE_FREQUENCY_CPM * enabledStationCount));
            if (currentTime - stateStartTime > cyclePeriodMs) {
                currentState = CycleState::READY;
            }
            break;
        }
    }
}

void updateStationCounters(int station, unsigned long cycles, int failures) {
    if (station < 0 || station >= STATION_COUNT) {
        reportEvent("Invalid station index for counter update: " + String(station));
        return;
    }
    
    cycleCount[station] = cycles;
    failureCount[station] = failures;
    
    reportEvent("Updated station " + String(station) + " counters: cycles=" + 
                String(cycles) + ", failures=" + String(failures));
}