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
constexpr int STATION_FAILURE_THRESHOLD = 10;

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
constexpr int KEYSWITCH_VOLTAGE_INPUT_PIN = A5;  // Connect the key switch current sensor to this pin
constexpr int STARTER_VOLTAGE_INPUT_PIN = A6;  // Connect the starter current sensor to this pin
constexpr float KEYSWITCH_CURRENT_THRESHOLD = 5.0;  // Minimum current for a successful cycle (in A)
constexpr float STARTER_CURRENT_THRESHOLD = 20.0;  // Minimum current for a successful cycle (in A)

// Current sensor specifications (based on 5V system)
constexpr float VOLTAGE_OFFSET_5V = 2.5;  // Original voltage offset for 5V system (in volts)
constexpr int KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR_5V = 16;  // Factor to convert voltage to current (A/V) https://www.phidgets.com/?prodid=1184
constexpr int STARTER_VOLTAGE_TO_CURRENT_FACTOR_5V = 48;  // Factor to convert voltage to current (A/V) https://www.phidgets.com/?prodid=1286

// Voltage divider scaling (5V to 3.3V)
constexpr float VOLTAGE_DIVIDER_RATIO = 2.0 / 3.0;  // Voltage divider ratio of 2k and 1k Ohm resistors

// Adjusted current factors for 3.3V system
constexpr float VOLTAGE_OFFSET = VOLTAGE_OFFSET_5V * VOLTAGE_DIVIDER_RATIO;  // Scaled voltage offset for 3.3V system
constexpr float KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR = KEYSWITCH_VOLTAGE_TO_CURRENT_FACTOR_5V / VOLTAGE_DIVIDER_RATIO;  // Adjusted for voltage divider
constexpr float STARTER_VOLTAGE_TO_CURRENT_FACTOR = STARTER_VOLTAGE_TO_CURRENT_FACTOR_5V / VOLTAGE_DIVIDER_RATIO;  // Adjusted for voltage divider

// ADC configuration
constexpr int ADC_RESOLUTION_BITS = 12;  // Set ADC resolution to 12 bits for better precision
constexpr int ADC_MAX_VALUE = 4095;  // Maximum value for 12-bit ADC (2^12 - 1)
constexpr float ADC_REFERENCE_VOLTAGE = 3.3;  // Reference voltage for ADC (in volts)

extern Dynamixel2Arduino servoBus;