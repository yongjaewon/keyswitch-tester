#include "commandParser.h"
#include "config.h"
#include "eventReporter.h"
#include "stateMachine.h"
#include "stationManager.h"
#include <Arduino.h>

void handleCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd == "START") {
            startSystem();
        } else if (cmd == "STOP") {
            stopSystem();
        } else if (cmd.startsWith("ENABLE:")) {
            enableStation(cmd.substring(7).toInt());
        } else if (cmd.startsWith("DISABLE:")) {
            disableStation(cmd.substring(8).toInt());
        } else if (cmd.startsWith("STATE:")) {
            processStateData(cmd.substring(6));
        } else if (cmd.startsWith("RESET_CYCLE:")) {
            resetStationCycles(cmd.substring(12).toInt());
        } else if (cmd.startsWith("RESET_FAIL:")) {
            resetStationFailures(cmd.substring(11).toInt());
        } else if (cmd == "REQUEST_STATE") {
            reportSystemState();
            reportStationStates();
        } else {
            reportEvent("Error: Unrecognized command - " + cmd);
        }
    }
}

void processStateData(const String& stateData) {
    // Expected format: "STATE:S0_CYCLES:S0_FAILS:S1_CYCLES:S1_FAILS:S2_CYCLES:S2_FAILS:S3_CYCLES:S3_FAILS"
    if (!stateData.startsWith("STATE:")) {
        reportEvent("Error: Invalid state data format");
        return;
    }
    
    String dataPart = stateData.substring(6); // Skip "STATE:"
    int values[STATION_COUNT * 2]; // Store cycles and failures for each station
    int valueIndex = 0;
    
    // Parse the string into values
    int startPos = 0;
    int colonPos = dataPart.indexOf(':', startPos);
    
    while (colonPos >= 0 && valueIndex < STATION_COUNT * 2) {
        values[valueIndex++] = dataPart.substring(startPos, colonPos).toInt();
        startPos = colonPos + 1;
        colonPos = dataPart.indexOf(':', startPos);
    }
    
    // Handle the last value
    if (valueIndex < STATION_COUNT * 2) {
        values[valueIndex++] = dataPart.substring(startPos).toInt();
    }
    
    // Check if we have all the values we need
    if (valueIndex != STATION_COUNT * 2) {
        reportEvent("Error: Incomplete state data received");
        return;
    }
    
    // Update the cycle and failure counts in the state machine
    for (int i = 0; i < STATION_COUNT; i++) {
        updateStationCounters(i, values[i * 2], values[i * 2 + 1]);
    }
    
    reportEvent("Successfully loaded state data from Raspberry Pi");
}

// Function to reset cycle count for a station
void resetStationCycles(int station) {
    resetStationCycleCount(station);
    reportEvent("Reset cycle count for station " + String(station));
}

// Function to reset failure count for a station
void resetStationFailures(int station) {    
    resetStationFailureCount(station);
    reportEvent("Reset failure count for station " + String(station));
}