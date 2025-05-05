//stateMachine.cpp
#include "stateMachine.h"
#include "config.h"
#include "eventReporter.h"
#include "currentMeasurer.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

// Using the ControlTableItem namespace for Dynamixel control table items
using namespace ControlTableItem;

static CycleState currentState = CycleState::STOPPED;
static bool stationEnabled[STATION_COUNT] = {true, true, true, true};  // Initialize all stations as enabled
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

// Function to handle the state machine
void handleStates() {
    unsigned long currentTime = millis();
    
    checkEmergencyStop();
    
    switch (currentState) {
        case CycleState::READY:
            // Start cycle if station is enabled
            if (stationEnabled[activeStationIndex]) {
                // Set up for movement
                stateStartTime = currentTime;
                
                // Reset peak current tracking
                resetPeakCurrent();
                
                // Move to next state
                currentState = CycleState::ACTUATING_START;
                reportEvent("Moving station " + String(activeStationIndex) + " to start position");
            } else {
                // Skip disabled station
                activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            }
            break;

        case CycleState::ACTUATING_START:            
            // Track peak currents during movement
            trackPeakCurrent();
            
            // Set goal position for the active station's servo
            servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_START, UNIT_DEGREE);

            // Wait for the configured time period
            if (currentTime - stateStartTime > ROTATE_TO_START_DURATION_MS) {
                // Movement time complete, move to complete state
                currentState = CycleState::ACTUATING_HOME;
                reportEvent("Station " + String(activeStationIndex) + " start movement time complete");
            }
            break;

        case CycleState::ACTUATING_HOME:
            // Track peak currents during movement
            trackPeakCurrent();
            
            // Return servo to released position
            servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_HOME, UNIT_DEGREE);

            // Wait for the configured time period
            if (currentTime - stateStartTime > ROTATE_TO_HOME_DURATION_MS) {
                // Movement time complete, move to complete state
                currentState = CycleState::PROCESSING;
                reportEvent("Station " + String(activeStationIndex) + " home movement time complete");
            }
            break;

        case CycleState::PROCESSING: {
            reportCurrent(activeStationIndex, getPeakCurrent());
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