//currentMeasurer.h
#pragma once

#include "config.h"

// Initialize the current measurement module
void initCurrentMeasurer();

// Reset peak current tracking
void resetPeakCurrents();

// Handle current measurement during movement
void handleMovementMeasurement(int stationIndex);

// Get the average of peak currents
float getAveragePeakCurrent();

// Check if current is below threshold (switch failure)
bool isCurrentBelowThreshold(float current); 