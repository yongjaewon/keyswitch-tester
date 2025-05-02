//stateMachine.h
#pragma once

enum class CycleState {
    HALTED,
    READY,
    ACTUATING_START,
    ACTUATING_HOME,
    PROCESSING,
    WAITING
};

void handleStates();
void enterSafeState();
void handleEmergencyStop();