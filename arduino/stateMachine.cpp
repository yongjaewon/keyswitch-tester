//stateMachine.cpp
#include "stateMachine.h"
#include "config.h"
#include "eventReporter.h"
#include "currentMeasurer.h"
#include "stationManager.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

//This namespace is required to use Dynamixel control table item names
using namespace ControlTableItem;

static CycleState currentState = CycleState::STOPPED;
static unsigned long stateStartTime = 0;
static int activeStationIndex = 0;

// Helper functions
void homeAllServos() {
    for (int i = 0; i < STATION_COUNT; i++) {
        servoBus.setGoalPosition(i, SERVO_ANGLE_HOME, UNIT_DEGREE);
    }
}

void checkEmergencyStop() {
    bool estopPressed = digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED;
    if (estopPressed && currentState != CycleState::EMERGENCY_STOPPED) {
        currentState = CycleState::EMERGENCY_STOPPED;
        reportEvent("Emergency stop detected");
    } else if (!estopPressed && currentState == CycleState::EMERGENCY_STOPPED) {
        currentState = CycleState::STOPPED;
        reportEvent("Emergency stop condition cleared");
    }
}

void processCycle() {
    incrementStationCycles(activeStationIndex);
    float keyswitchCurrent = getPeakKeyswitchCurrent();
    float starterCurrent = getPeakStarterCurrent();
    if (keyswitchCurrent < KEYSWITCH_CURRENT_THRESHOLD || starterCurrent < STARTER_CURRENT_THRESHOLD) {
        incrementStationFailures(activeStationIndex);
    }
    if (getStationFailures(activeStationIndex) >= STATION_FAILURE_THRESHOLD) {
        disableStation(activeStationIndex);
    }
    reportCycle(
        activeStationIndex,
        isStationEnabled(activeStationIndex),
        getStationCycles(activeStationIndex),
        getStationFailures(activeStationIndex),
        keyswitchCurrent,
        starterCurrent
    );
    activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
    currentState = CycleState::WAITING;
}

void handleStates() {
    unsigned long currentTime = millis();
    
    // Check emergency stop first
    checkEmergencyStop();
    
    switch (currentState) {
        case CycleState::EMERGENCY_STOPPED:
        case CycleState::STOPPED:
            homeAllServos();
            break;
            
        case CycleState::READY:
            if (isStationEnabled(activeStationIndex)) {
                stateStartTime = currentTime;
                calibrateOffsets();
                resetPeakCurrents();
                currentState = CycleState::ACTUATING;
            } else {
                activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            }
            break;
            
        case CycleState::ACTUATING: {
            trackPeakCurrents();
            unsigned long elapsedTime = currentTime - stateStartTime;
            
            if (elapsedTime <= ROTATE_TO_START_DURATION_MS) {
                servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_START, UNIT_DEGREE);
            } else if (elapsedTime <= ROTATE_TO_START_DURATION_MS + ROTATE_TO_HOME_DURATION_MS) {
                servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_HOME, UNIT_DEGREE);
            } else {
                processCycle();
            }
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

// System control functions
bool startSystem() {
    if (currentState == CycleState::STOPPED || currentState == CycleState::EMERGENCY_STOPPED) {
        if (digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED) {
            reportEvent("Unable to start system. Emergency stop condition detected.");
            return false;
        }
        currentState = CycleState::READY;
        reportEvent("System started.");
        reportSystemState();
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
    reportSystemState();
    return true;
}

CycleState getCurrentState() {
    return currentState;
}

bool isSystemRunning() {
    return (currentState != CycleState::STOPPED && currentState != CycleState::EMERGENCY_STOPPED);
}