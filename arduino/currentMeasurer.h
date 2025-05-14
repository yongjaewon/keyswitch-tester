//currentMeasurer.h
#pragma once
#include "config.h"

// Calibration samples to take for initial calibration
constexpr int CALIBRATION_SAMPLES = 100;

// Function declarations
void trackPeakCurrents();
float getPeakKeyswitchCurrent();
float getPeakStarterCurrent();
void resetPeakCurrents();
void calibrateOffsets();