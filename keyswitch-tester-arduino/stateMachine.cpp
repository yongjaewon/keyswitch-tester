//stateMachine.cpp
#include "stateMachine.h"
#include "config.h"
#include "eventReporter.h"
#include "currentMeasurer.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

// Using the ControlTableItem namespace for Dynamixel control table items
using namespace ControlTableItem;

static CycleState currentState = CycleState::READY;
static unsigned long stateStartTime = 0;
static unsigned long waitDurationMs = 0;

int getEnabledStationCount() {
    int count = 0;
    for (int i = 0; i < STATION_COUNT; i++) {
        if (stationEnabled[i]) {
            count++;
        }
    }
    return count;
}

// Function to handle the state machine
void handleStates() {
    unsigned long currentTime = millis();
    
    // Check for emergency stop condition
    if (digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED) {
        for (int i = 0; i < STATION_COUNT; i++) {
            servoBus.setGoalPosition(i, SERVO_ANGLE_HOME, UNIT_DEGREE);
        }
        masterEnabled = false;
        currentState = CycleState::HALTED;
        reportEvent("Emergency stop condition detected");
        return;
    }
    
    switch (currentState) {
        case CycleState::HALTED:
            if (digitalRead(EMERGENCY_STOP_PIN) == ESTOP_RELEASED) {
                currentState = CycleState::READY;
            }
            break;
            
        case CycleState::READY:
            // Start cycle if station is enabled
            if (stationEnabled[activeStationIndex]) {
                // Set up for movement
                stateStartTime = currentTime;
                
                // Reset peak current tracking
                resetPeakCurrents();
                
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
            handleMovementMeasurement(activeStationIndex);
            
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
            handleMovementMeasurement(activeStationIndex);
            
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
            // Calculate average of peak currents
            float avgPeakCurrent = getAveragePeakCurrent();
            reportEvent("Station " + String(activeStationIndex) + " average peak current: " + String(avgPeakCurrent));
            
            // Check if current is below threshold (switch failure)
            if (isCurrentBelowThreshold(avgPeakCurrent)) {
                reportEvent("Station " + String(activeStationIndex) + " failed: current below threshold");
            }
            
            // Advance to next station
            activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            
            // Reset state
            currentState = CycleState::WAITING;

            reportEvent("Cycle complete, moving to station " + String(activeStationIndex));
            break;
        }

        case CycleState::WAITING: {
            // Wait for the configured time period
            int enabledStationCount = getEnabledStationCount();
            int cyclePeriodMs = (enabledStationCount == 0) ? 0 : (60000 / (CYCLE_FREQUENCY_CPM * enabledStationCount));

            if (currentTime - stateStartTime > cyclePeriodMs) {
                currentState = CycleState::READY;
            }
            break;
        }
    }
}