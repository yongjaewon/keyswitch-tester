//currentMeasurer.cpp
#include "currentMeasurer.h"
#include <Arduino.h>

// Peak current tracking (from hall effect sensor)
static float peakCurrents[MAX_PEAK_CURRENTS] = {0};
static int peakCurrentCount = 0;
static int lowestPeakIndex = 0;  // Track the index of the lowest peak

// Initialize the current measurement module
void initCurrentMeasurer() {
    pinMode(VOLTAGE_INPUT_PIN, INPUT);
    resetPeakCurrents();
}

// Read voltage from analog pin and convert to current
static float readVoltageAndConvertToCurrent() {
    // Read analog value and convert to voltage (assuming 5V reference)
    float voltage = (analogRead(VOLTAGE_INPUT_PIN) * 5.0) / 1023.0 - VOLTAGE_OFFSET;
    
    // Convert voltage to current using the formula
    return voltage * VOLTAGE_TO_CURRENT_FACTOR;
}

// Update peak currents with a new reading
static void updatePeakCurrents(float current) {
    // If we haven't filled the array yet, just add the new value
    if (peakCurrentCount < MAX_PEAK_CURRENTS) {
        peakCurrents[peakCurrentCount] = current;
        
        // Update lowest peak index if this new value is lower
        if (current < peakCurrents[lowestPeakIndex]) {
            lowestPeakIndex = peakCurrentCount;
        }
        
        peakCurrentCount++;
        return;
    }
    
    // If the new current is higher than the lowest, replace it
    if (current > peakCurrents[lowestPeakIndex]) {
        peakCurrents[lowestPeakIndex] = current;
        
        // Find the new lowest value in the array
        lowestPeakIndex = 0;
        for (int i = 1; i < MAX_PEAK_CURRENTS; i++) {
            if (peakCurrents[i] < peakCurrents[lowestPeakIndex]) {
                lowestPeakIndex = i;
            }
        }
    }
}

// Reset peak current tracking
void resetPeakCurrents() {
    peakCurrentCount = 0;
    lowestPeakIndex = 0;
    for (int i = 0; i < MAX_PEAK_CURRENTS; i++) {
        peakCurrents[i] = 0;
    }
}

// Get the average of peak currents
float getAveragePeakCurrent() {
    if (peakCurrentCount == 0) return 0.0;
    
    float sum = 0.0;
    for (int i = 0; i < peakCurrentCount; i++) {
        sum += peakCurrents[i];
    }
    
    return sum / peakCurrentCount;
}

// Check if current is below threshold (switch failure)
bool isCurrentBelowThreshold(float current) {
    return current < MIN_SWITCH_CURRENT;
}

// Handle current measurement during movement
void handleMovementMeasurement(int stationIndex) {
    float current = readVoltageAndConvertToCurrent();
    
    // Track peak currents during movement
    updatePeakCurrents(current);
    
    // Check if current is below threshold (switch failure)
    if (isCurrentBelowThreshold(current)) {
        // This function doesn't directly modify switchFailCount
        // The state machine will handle that based on the return value
    }
} 