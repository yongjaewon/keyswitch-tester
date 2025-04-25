//stateMachine.cpp
#include "stateMachine.h"
#include "config.h"
#include <Dynamixel2Arduino.h>

enum class CycleState {
    Idle,
    MovingToPosition,
    Measuring,
    Complete,
    Error
};

static CycleState currentState = CycleState::Idle;

void handleStates() {
    switch (currentState) {
        case CycleState::Idle:
            // Example: start if station is enabled
            if (stationEnabled[activeStationIndex]) {
                // prepare for next step
                currentState = CycleState::MovingToPosition;
            }
            break;

        case CycleState::MovingToPosition:
            // Send goal position, check for completion
            // If done:
            currentState = CycleState::Measuring;
            break;

        case CycleState::Measuring:
            // Measure current
            // If pass/fail:
            currentState = CycleState::Complete;
            break;

        case CycleState::Complete:
            // Advance to next station, reset state
            activeStationIndex = (activeStationIndex + 1) % STATION_COUNT;
            currentState = CycleState::Idle;
            break;

        case CycleState::Error:
            // Stay here or reset
            break;
    }
}