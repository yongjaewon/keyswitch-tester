#include "config.h"
#include "stateMachine.h"
#include "eventReporter.h"
#include "commandParser.h"
#include "currentMeasurer.h"
#include "framStorage.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>
#include <SPI.h>

Dynamixel2Arduino servoBus(SERVO_SERIAL, SERVO_DIR_PIN);

//This namespace is required to use Dynamixel control table item names
using namespace ControlTableItem;

// Track the last FRAM backup time
unsigned long lastFramBackupTime = 0;

void setup() {
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);

  analogReadResolution(ADC_RESOLUTION_BITS);

  pinMode(KEYSWITCH_VOLTAGE_INPUT_PIN, INPUT);
  pinMode(STARTER_VOLTAGE_INPUT_PIN, INPUT);

  // USB connection with Raspberry Pi
  Serial.begin(115200);
  while (!Serial) {
    // Wait for Raspberry Pi to connect
    delay(500);
  }

  // Initialize SPI for FRAM
  SPI.begin();
  
  // Set servo bus port baudrate to 57600bps and the protocol version to 2.0. This has to match with DYNAMIXEL.
  servoBus.begin(57600);
  servoBus.setPortProtocolVersion(SERVO_PROTOCOL_VERSION);

  // Initialize and home all servos
  for (int i = 0; i < STATION_COUNT; i++) {
    servoBus.torqueOff(i);
    servoBus.setOperatingMode(i, OP_CURRENT_BASED_POSITION);
    servoBus.torqueOn(i);
    servoBus.setGoalCurrent(i, SERVO_MAX_TORQUE_PERCENT, UNIT_PERCENT);
    servoBus.setGoalPosition(i, SERVO_ANGLE_HOME, UNIT_DEGREE);
  }
  
  // Wait a moment for serial connection to stabilize
  delay(500);
  
  // Initialize FRAM and load saved station states
  if (initFRAM()) {
    loadStationStates();
    reportEvent("Loaded station states from FRAM");
  } else {
    reportEvent("Failed to initialize FRAM, using default station states");
  }
  
  // Save current time for backup timing
  lastFramBackupTime = millis();
  
  // Perform initial current measurement calibration
  calibrateOffsets();
}

void loop() {
  handleCommands();
  handleStates();
  
  // Periodically backup all states to FRAM to ensure data integrity
  unsigned long currentTime = millis();
  if (currentTime - lastFramBackupTime >= FRAM_BACKUP_INTERVAL_MS) {
    saveStationStates();
    lastFramBackupTime = currentTime;
  }
}