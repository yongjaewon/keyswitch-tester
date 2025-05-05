#include "currentMeasurer.h"
#include <Arduino.h>

static float peakCurrent = 0.0;

void trackPeakCurrent() {
    float voltage = (analogRead(VOLTAGE_INPUT_PIN) * 5.0) / 1023.0 - VOLTAGE_OFFSET;
    float current = voltage * VOLTAGE_TO_CURRENT_FACTOR;
    if (current > peakCurrent) peakCurrent = current;
}

float getPeakCurrent() {
    return peakCurrent;
}

void resetPeakCurrent() {
    peakCurrent = 0.0;
}
