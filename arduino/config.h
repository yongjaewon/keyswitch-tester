//config.h
#pragma once
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

// Emergency stop configuration
#define ESTOP_PRESSED HIGH
#define ESTOP_RELEASED LOW
constexpr int EMERGENCY_STOP_PIN = 0;  // Digital pin 0 for emergency stop input

// Station configuration
constexpr int STATION_COUNT = 4;

// Servo configuration
#define SERVO_SERIAL Serial1
constexpr int SERVO_DIR_PIN = -1;
constexpr float SERVO_PROTOCOL_VERSION = 2.0;
constexpr int SERVO_ANGLE_HOME = 0;
constexpr int SERVO_ANGLE_START = 90;  // Set higher than needed to ensure the key rotates fully. The torque limit will prevent stalling.
constexpr int SERVO_MAX_TORQUE_PERCENT = 10;
constexpr unsigned long ROTATE_TO_START_DURATION_MS = 500; // This is longer to keep the key switch in the start position for some time.
constexpr unsigned long ROTATE_TO_HOME_DURATION_MS = 200;
constexpr int CYCLE_FREQUENCY_CPM = 6;

// Current measurement configuration
constexpr float VOLTAGE_OFFSET = 2.5;  // Voltage offset for current calculation (in volts)
constexpr int KEYSWITCH_VOLTAGE_INPUT_PIN = A0;  // Connect the key switch current sensor to this pin
constexpr int STARTER_VOLTAGE_INPUT_PIN = A1;  // Connect the starter current sensor to this pin
constexpr float KEYSWITCH_CURRENT_THRESHOLD = 5.0;  // Minimum current for a successful cycle (in A)
constexpr float STARTER_CURRENT_THRESHOLD = 20.0;  // Minimum current for a successful cycle (in A)
constexpr int KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR = 16;  // Factor to convert voltage to current (A/V) https://www.phidgets.com/?prodid=1184
constexpr int STARTER_VOLTAGE_TO_CURRENT_FACTOR = 48;  // Factor to convert voltage to current (A/V) https://www.phidgets.com/?prodid=1286

extern Dynamixel2Arduino servoBus;