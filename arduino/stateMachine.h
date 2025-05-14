//stateMachine.h
#pragma once
#include <Arduino.h>

enum class CycleState {
    EMERGENCY_STOPPED,
    STOPPED,
    READY,
    ACTUATING,
    WAITING
};

void handleStates();
void checkEmergencyStop();
bool startSystem();
bool stopSystem();
CycleState getCurrentState();
bool isSystemRunning();