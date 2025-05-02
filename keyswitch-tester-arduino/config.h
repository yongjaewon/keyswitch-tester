//config.h
#pragma once
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

// Emergency stop configuration
#define ESTOP_PRESSED HIGH
#define ESTOP_RELEASED LOW
constexpr int EMERGENCY_STOP_PIN = 0;  // Digital pin 0 (PA22) for emergency stop input

constexpr int STATION_COUNT = 4;
constexpr int SWITCH_FAILURE_THRESHOLD = 10;

// Servo configuration
#define SERVO_SERIAL Serial1
constexpr int SERVO_DIR_PIN = -1;
constexpr float SERVO_PROTOCOL_VERSION = 2.0;
constexpr int SERVO_ANGLE_HOME = 0;  // Angle when the key is at home
constexpr int SERVO_ANGLE_START = 90;  // Angle when the key is at the start position
constexpr int SERVO_MAX_TORQUE_PERCENT = 10;  // Maximum torque percentage (0-100)

// Movement configuration
constexpr int CYCLE_FREQUENCY_CPM = 6;  // Frequency of the cycle in cycles per minute
constexpr unsigned long ROTATE_TO_START_DURATION_MS = 500;  // Duration of the move in milliseconds
constexpr unsigned long ROTATE_TO_HOME_DURATION_MS = 200;  // Duration of the move in milliseconds

// Current measurement configuration
constexpr int MAX_PEAK_CURRENTS = 10;  // Maximum number of peak currents to track
constexpr float MIN_SWITCH_CURRENT = 1.0;  // Minimum current threshold for switch failure (in amperes)
constexpr float VOLTAGE_TO_CURRENT_FACTOR = 0.1;  // Factor to convert voltage to current (A/V)
constexpr float VOLTAGE_OFFSET = 2.5;  // Voltage offset for current calculation (in volts)
constexpr int VOLTAGE_INPUT_PIN = A0;  // Analog pin A0 (PA03/AIN[1]) for voltage input

extern Dynamixel2Arduino servoBus;
extern bool masterEnabled;
extern bool stationEnabled[STATION_COUNT];
extern int activeStationIndex;