//framStorage.h
#pragma once
#include <Arduino.h>
#include <Adafruit_FRAM_SPI.h>
#include "config.h"

// Structure to hold station state data
struct StationData {
    uint32_t cycleCount;       // 4 bytes - cycle count
    uint16_t failureCount;     // 2 bytes - failure count  
    uint8_t enabled;           // 1 byte - enabled state (0=disabled, 1=enabled)
    uint8_t reserved;          // 1 byte - reserved for future use
};

// Initialize FRAM module
bool initFRAM();

// Load station states from FRAM
bool loadStationStates();

// Save all station states to FRAM
bool saveStationStates();

// Save a specific station state to FRAM
bool saveStationState(uint8_t stationIndex);

// Save cycle count for a specific station
bool saveStationCycles(uint8_t stationIndex, uint32_t cycles);

// Save failure count for a specific station
bool saveStationFailures(uint8_t stationIndex, uint16_t failures);

// Save enabled state for a specific station
bool saveStationEnabled(uint8_t stationIndex, bool enabled);

// Function to reset the FRAM data (clear all stored data)
bool resetFRAM();

// Gets the size in bytes of the FRAM memory
uint32_t getFRAMSize();

// Test function to verify FRAM is working
bool testFRAM(); 