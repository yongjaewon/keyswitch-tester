#include "currentMeasurer.h"
#include <Arduino.h>

static float peakKeyswitchCurrent = 0.0;
static float peakStarterCurrent = 0.0;
static int keyswitchOffset = 0;  // Raw ADC value
static int starterOffset = 0;    // Raw ADC value

void calibrateOffsets() {
    long keyswitchSum = 0;
    long starterSum = 0;
    
    // Take multiple samples and average them
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        keyswitchSum += analogRead(KEYSWITCH_VOLTAGE_INPUT_PIN);
        starterSum += analogRead(STARTER_VOLTAGE_INPUT_PIN);
        delay(1);  // Small delay between readings
    }
    
    // Store raw ADC offsets
    keyswitchOffset = keyswitchSum / CALIBRATION_SAMPLES;
    starterOffset = starterSum / CALIBRATION_SAMPLES;
}

void trackPeakCurrents() {
    // Read raw ADC values
    int keyswitchRaw = analogRead(KEYSWITCH_VOLTAGE_INPUT_PIN);
    int starterRaw = analogRead(STARTER_VOLTAGE_INPUT_PIN);

    // Convert to voltage after subtracting offset
    float keyswitchVoltage = ((keyswitchRaw - keyswitchOffset) * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
    float keyswitchCurrent = abs(keyswitchVoltage) * KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR;
    if (keyswitchCurrent > peakKeyswitchCurrent) peakKeyswitchCurrent = keyswitchCurrent;

    float starterVoltage = ((starterRaw - starterOffset) * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
    float starterCurrent = abs(starterVoltage) * STARTER_VOLTAGE_TO_CURRENT_FACTOR;
    if (starterCurrent > peakStarterCurrent) peakStarterCurrent = starterCurrent;
}

float getPeakKeyswitchCurrent() {
    return peakKeyswitchCurrent;
}

float getPeakStarterCurrent() {
    return peakStarterCurrent;
}

void resetPeakCurrents() {
    peakKeyswitchCurrent = 0.0;
    peakStarterCurrent = 0.0;
}
