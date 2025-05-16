//framStorage.cpp
#include "framStorage.h"
#include "eventReporter.h"
#include "stationManager.h"

// Define SPI FRAM instance
// Using hardware SPI with CS pin only
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS_PIN);

// Size of the FRAM memory in bytes (4 Mbit = 512 KB)
constexpr uint32_t FRAM_SIZE = 524288;

bool initFRAM() {
    // Initialize the FRAM module
    if (!fram.begin()) {
        reportEvent("Error: Failed to initialize FRAM module");
        return false;
    }
    
    // Check if FRAM has been initialized by checking the magic number
    uint32_t magicNumber = 0;
    fram.read(FRAM_MAGIC_NUMBER_ADDR, (uint8_t*)&magicNumber, sizeof(magicNumber));
    
    if (magicNumber != FRAM_MAGIC_NUMBER) {
        // FRAM has not been initialized, so initialize it now
        reportEvent("Initializing FRAM storage for the first time");
        
        // Write magic number
        uint32_t magicNum = FRAM_MAGIC_NUMBER;
        fram.writeEnable(true);
        fram.write(FRAM_MAGIC_NUMBER_ADDR, (uint8_t*)&magicNum, sizeof(magicNum));
        fram.writeEnable(false);
        
        // Write version
        uint16_t version = FRAM_DATA_VERSION;
        fram.writeEnable(true);
        fram.write(FRAM_VERSION_ADDR, (uint8_t*)&version, sizeof(version));
        fram.writeEnable(false);
        
        // Initialize with default values (all zeros)
        for (uint8_t i = 0; i < STATION_COUNT; i++) {
            StationData data = {0, 0, 1, 0};  // Default: cycles=0, failures=0, enabled=true, reserved=0
            saveStationState(i);
        }
        
        reportEvent("FRAM initialized successfully");
    } else {
        // Check version for future compatibility
        uint16_t version = 0;
        fram.read(FRAM_VERSION_ADDR, (uint8_t*)&version, sizeof(version));
        
        if (version != FRAM_DATA_VERSION) {
            reportEvent("Warning: FRAM data version mismatch - attempting to load anyway");
            // In the future, you might add migration code here
        }
        
        reportEvent("FRAM connection verified");
    }
    
    return true;
}

bool loadStationStates() {
    reportEvent("Loading station states from FRAM");
    
    // Check if FRAM is initialized
    uint32_t magicNumber = 0;
    fram.read(FRAM_MAGIC_NUMBER_ADDR, (uint8_t*)&magicNumber, sizeof(magicNumber));
    
    if (magicNumber != FRAM_MAGIC_NUMBER) {
        reportEvent("Error: FRAM not initialized, cannot load states");
        return false;
    }
    
    // Check version for compatibility
    uint16_t version = 0;
    fram.read(FRAM_VERSION_ADDR, (uint8_t*)&version, sizeof(version));
    
    if (version != FRAM_DATA_VERSION) {
        reportEvent("Warning: FRAM data version mismatch - attempting to load anyway");
    }
    
    // Load station data
    bool anyStationLoaded = false;
    
    for (uint8_t i = 0; i < STATION_COUNT; i++) {
        StationData data;
        uint32_t addr = FRAM_STATION_DATA_ADDR + (i * sizeof(StationData));
        
        fram.read(addr, (uint8_t*)&data, sizeof(StationData));
        
        // Validate data integrity - basic plausibility checks
        bool dataValid = true;
        
        // Detect if data is likely corrupted (using simple heuristics)
        if (data.enabled > 1) {  // Enabled should be 0 or 1
            dataValid = false;
        }
        
        // Some arbitrary reasonable limits for cycle and failure counts
        if (data.cycleCount > 10000000 || data.failureCount > 10000) {
            reportEvent("Warning: Station " + String(i) + " has suspicious counter values");
            // Not failing validation since these could technically be valid in some cases
        }
        
        if (dataValid) {
            // Update station manager with loaded data
            updateStationCounters(i, data.cycleCount, data.failureCount);
            
            if (data.enabled) {
                enableStation(i);
            } else {
                disableStation(i);
            }
            
            anyStationLoaded = true;
        } else {
            reportEvent("Warning: Invalid data detected for station " + String(i) + ", using defaults");
            // Use default values
            updateStationCounters(i, 0, 0);
            enableStation(i);
        }
    }
    
    if (anyStationLoaded) {
        reportEvent("Station states loaded successfully");
        return true;
    } else {
        reportEvent("No valid station data found in FRAM");
        return false;
    }
}

bool saveStationStates() {
    reportEvent("Saving all station states to FRAM");
    
    for (uint8_t i = 0; i < STATION_COUNT; i++) {
        if (!saveStationState(i)) {
            return false;
        }
    }
    
    return true;
}

bool saveStationState(uint8_t stationIndex) {
    if (stationIndex >= STATION_COUNT) {
        return false;
    }
    
    StationData data;
    data.cycleCount = getStationCycles(stationIndex);
    data.failureCount = getStationFailures(stationIndex);
    data.enabled = isStationEnabled(stationIndex) ? 1 : 0;
    data.reserved = 0;
    
    uint32_t addr = FRAM_STATION_DATA_ADDR + (stationIndex * sizeof(StationData));
    
    // Write data to FRAM
    fram.writeEnable(true);
    fram.write(addr, (uint8_t*)&data, sizeof(StationData));
    fram.writeEnable(false);
    
    // Verify data was written correctly (read back and compare)
    StationData verifyData;
    fram.read(addr, (uint8_t*)&verifyData, sizeof(StationData));
    
    if (verifyData.cycleCount != data.cycleCount || 
        verifyData.failureCount != data.failureCount || 
        verifyData.enabled != data.enabled) {
        reportEvent("Error: FRAM verification failed for station " + String(stationIndex));
        return false;
    }
    
    return true;
}

bool saveStationCycles(uint8_t stationIndex, uint32_t cycles) {
    if (stationIndex >= STATION_COUNT) {
        return false;
    }
    
    uint32_t addr = FRAM_STATION_DATA_ADDR + (stationIndex * sizeof(StationData));
    
    fram.writeEnable(true);
    fram.write(addr, (uint8_t*)&cycles, sizeof(cycles));
    fram.writeEnable(false);
    
    return true;
}

bool saveStationFailures(uint8_t stationIndex, uint16_t failures) {
    if (stationIndex >= STATION_COUNT) {
        return false;
    }
    
    uint32_t addr = FRAM_STATION_DATA_ADDR + (stationIndex * sizeof(StationData)) + sizeof(uint32_t);
    
    fram.writeEnable(true);
    fram.write(addr, (uint8_t*)&failures, sizeof(failures));
    fram.writeEnable(false);
    
    return true;
}

bool saveStationEnabled(uint8_t stationIndex, bool enabled) {
    if (stationIndex >= STATION_COUNT) {
        return false;
    }
    
    uint8_t enabledVal = enabled ? 1 : 0;
    uint32_t addr = FRAM_STATION_DATA_ADDR + (stationIndex * sizeof(StationData)) + sizeof(uint32_t) + sizeof(uint16_t);
    
    fram.writeEnable(true);
    fram.write(addr, &enabledVal, sizeof(enabledVal));
    fram.writeEnable(false);
    
    return true;
}

bool resetFRAM() {
    reportEvent("Resetting FRAM data to defaults");
    
    // Reset magic number and version
    uint32_t magicNum = FRAM_MAGIC_NUMBER;
    fram.writeEnable(true);
    fram.write(FRAM_MAGIC_NUMBER_ADDR, (uint8_t*)&magicNum, sizeof(magicNum));
    fram.writeEnable(false);
    
    uint16_t version = FRAM_DATA_VERSION;
    fram.writeEnable(true);
    fram.write(FRAM_VERSION_ADDR, (uint8_t*)&version, sizeof(version));
    fram.writeEnable(false);
    
    // Reset all station data
    for (uint8_t i = 0; i < STATION_COUNT; i++) {
        StationData data = {0, 0, 1, 0};  // Default: cycles=0, failures=0, enabled=true, reserved=0
        uint32_t addr = FRAM_STATION_DATA_ADDR + (i * sizeof(StationData));
        
        fram.writeEnable(true);
        fram.write(addr, (uint8_t*)&data, sizeof(StationData));
        fram.writeEnable(false);
    }
    
    reportEvent("FRAM reset complete");
    return true;
}

uint32_t getFRAMSize() {
    return FRAM_SIZE;
}

bool testFRAM() {
    reportEvent("Testing FRAM...");
    
    // Write test pattern to a safe location (after station data)
    uint32_t testAddr = FRAM_STATION_DATA_ADDR + (STATION_COUNT * sizeof(StationData));
    uint32_t testPattern = 0xAA55AA55;
    
    // Write test pattern
    fram.writeEnable(true);
    fram.write(testAddr, (uint8_t*)&testPattern, sizeof(testPattern));
    fram.writeEnable(false);
    
    // Read back
    uint32_t readPattern = 0;
    fram.read(testAddr, (uint8_t*)&readPattern, sizeof(readPattern));
    
    if (readPattern != testPattern) {
        reportEvent("FRAM test failed! Write/read mismatch");
        return false;
    }
    
    // Write inverted pattern
    testPattern = ~testPattern;
    fram.writeEnable(true);
    fram.write(testAddr, (uint8_t*)&testPattern, sizeof(testPattern));
    fram.writeEnable(false);
    
    // Read back
    readPattern = 0;
    fram.read(testAddr, (uint8_t*)&readPattern, sizeof(readPattern));
    
    if (readPattern != testPattern) {
        reportEvent("FRAM test failed! Write/read mismatch");
        return false;
    }
    
    reportEvent("FRAM test passed successfully");
    return true;
} 