#include "config.h"
#include "stateMachine.h"
#include "eventReporter.h"
#include "commandParser.h"
#include "currentMeasurer.h"
#include <Dynamixel2Arduino.h>
#include <Arduino.h>

Dynamixel2Arduino servoBus(SERVO_SERIAL, SERVO_DIR_PIN);

//This namespace is required to use Dynamixel control table item names
using namespace ControlTableItem;

// Initialize global variables
bool masterEnabled = false;
bool stationEnabled[STATION_COUNT] = {true, true, true, true};  // Initialize all stations as enabled
int activeStationIndex = 0;

void setup() {
  // Initialize emergency stop pin with pull-up resistor
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);

  // USB connection with Raspberry Pi
  Serial.begin(115200);

  // Set servo bus port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  servoBus.begin(57600);

  // Set servo bus port protocol version. This has to match with DYNAMIXEL protocol version.
  servoBus.setPortProtocolVersion(SERVO_PROTOCOL_VERSION);

  // Initialize all servos
  for (int i = 0; i < STATION_COUNT; i++) {
    servoBus.torqueOff(i);
    servoBus.setOperatingMode(i, OP_CURRENT_BASED_POSITION);
    servoBus.torqueOn(i);
    servoBus.setGoalCurrent(i, SERVO_MAX_TORQUE_PERCENT, UNIT_PERCENT);
    servoBus.setGoalPosition(i, SERVO_ANGLE_HOME, UNIT_DEGREE);
  }
  
  // Initialize the current measurement module
  initCurrentMeasurer();
}

void loop() {
  handleCommands();
  handleStates();
}