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

void enterSafeState() {
    for (int i = 0; i < STATION_COUNT; i++) {
        servoBus.torqueOn(i);
        servoBus.setGoalCurrent(i, SERVO_MAX_TORQUE_PERCENT);
        servoBus.setGoalPosition(i, SERVO_ANGLE_HOME);
    }
    currentState = CycleState::HALTED;
}

// Function to handle the state machine
void handleStates() {
    unsigned long currentTime = millis();
    
    // Check for emergency stop condition
    if (digitalRead(EMERGENCY_STOP_PIN) == ESTOP_PRESSED) {
        if (currentState != CycleState::HALTED) {
            enterSafeState();
        }
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
                
                // Set goal position for the active station's servo
                servoBus.setGoalCurrent(activeStationIndex, SERVO_MAX_TORQUE_PERCENT);
                servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_START);
                
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
            
            // Wait for the configured time period
            if (currentTime - stateStartTime > MOVE_DURATION_MS) {
                // Movement time complete, move to complete state
                currentState = CycleState::PROCESSING;
                reportEvent("Station " + String(activeStationIndex) + " movement time complete");
            }
            break;

        case CycleState::PROCESSING:
            // Calculate average of peak currents
            float avgPeakCurrent = getAveragePeakCurrent();
            reportEvent("Station " + String(activeStationIndex) + " average peak current: " + String(avgPeakCurrent));
            
            // Check if current is below threshold (switch failure)
            if (isCurrentBelowThreshold(avgPeakCurrent)) {
                reportEvent("Station " + String(activeStationIndex) + " failed: current below threshold");
            }
            
            // Return servo to released position
            servoBus.setGoalPosition(activeStationIndex, SERVO_ANGLE_HOME);
            
            // Advance to next station
            activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            
            // Reset state
            currentState = CycleState::READY;
            reportEvent("Cycle complete, moving to station " + String(activeStationIndex));
            break;
    }
}