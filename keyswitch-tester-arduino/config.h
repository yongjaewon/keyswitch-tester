//config.h
#pragma once

#define DXL_SERIAL Serial1

constexpr int DXL_DIR_PIN = -1;
constexpr float DXL_PROTOCOL_VERSION = 2.0;

constexpr int STATION_COUNT = 4;
constexpr float MIN_SWITCH_CURRENT = 5.0;
constexpr int SWITCH_FAILURE_THRESHOLD = 10;
constexpr float SERVO_MAX_TORQUE_PERCENT = 10.0;

extern bool masterEnabled;
extern bool stationEnabled[STATION_COUNT];
extern int switchFailCount[STATION_COUNT];
extern int activeStationIndex;