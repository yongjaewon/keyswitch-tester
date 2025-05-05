//stateMachine.h
#pragma once

enum class CycleState {
    EMERGENCY_STOPPED,
    STOPPED,
    READY,
    ACTUATING_START,
    ACTUATING_HOME,
    PROCESSING,
    WAITING
};

// Function declarations
void handleStates();
void checkEmergencyStop();
bool enableStation(int station);
bool disableStation(int station);
bool startSystem();
bool stopSystem();
int getEnabledStationCount();