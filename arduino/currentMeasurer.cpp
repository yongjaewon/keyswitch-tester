#include "currentMeasurer.h"
#include <Arduino.h>

static float peakKeyswitchCurrent = 0.0;
static float peakStarterCurrent = 0.0;

void trackPeakCurrents() {
    float keyswitchVoltage = (analogRead(KEYSWITCH_VOLTAGE_INPUT_PIN) * 5.0) / 1023.0 - VOLTAGE_OFFSET;
    float keyswitchCurrent = abs(keyswitchVoltage) * KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR;
    if (keyswitchCurrent > peakKeyswitchCurrent) peakKeyswitchCurrent = keyswitchCurrent;

    float starterVoltage = (analogRead(STARTER_VOLTAGE_INPUT_PIN) * 5.0) / 1023.0 - VOLTAGE_OFFSET;
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
