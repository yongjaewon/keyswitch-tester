#include "config.h"
#include "stateMachine.h"
#include "eventReporter.h"
#include "commandParser.h"
#include <Dynamixel2Arduino.h>

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

//This namespace is required to use Dynamixel control table item names
using namespace ControlTableItem;

bool masterEnabled = false;
bool stationEnabled[STATION_COUNT] = {};
int switchFailCount[STATION_COUNT] = {};
int activeStationIndex = -1;

void setup() {
  // put your setup code here, to run once:

  // USB connection with Raspberry Pi
  Serial.begin(115200);

  // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(57600);

  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  // Turn off torque when configuring items in EEPROM area
  for (int i = 0; i < STATION_COUNT; i++) {
    dxl.torqueOff(i);
    dxl.setOperatingMode(i, OP_CURRENT_BASED_POSITION);
    dxl.torqueOn(i);
    dxl.setGoalCurrent(i, SERVO_MAX_TORQUE_PERCENT, UNIT_PERCENT);
  }
}

void loop() {
  handleCommands();

  if (masterEnabled) handleStates();
  else enterSafeState();
}

void enterSafeState() {
  for (int i = 0; i < STATION_COUNT; i++) {
    dxl.setGoalPosition(i, 0);
  }
  delay(1000);
}