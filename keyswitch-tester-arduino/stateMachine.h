//stateMachine.h
#pragma once

enum class CycleState {
    READY,
    ACTUATING_START,
    ACTUATING_HOME,
    PROCESSING,
    HALTED
};

void handleStates();
void enterSafeState();
void handleEmergencyStop();