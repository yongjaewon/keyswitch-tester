/*******************************************************************************
* Copyright 2016 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <Dynamixel2Arduino.h>

#define DXL_SERIAL Serial1
#define DEBUG_SERIAL Serial
const int DXL_DIR_PIN = -1;

const float DXL_PROTOCOL_VERSION = 2.0;

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

//This namespace is required to use Control table item names
using namespace ControlTableItem;

void setup() {
  // put your setup code here, to run once:

  // Use Serial to debug.
  DEBUG_SERIAL.begin(115200);
  
  // Wait for serial monitor to be connected
  while (!DEBUG_SERIAL) {
    ; // Wait for serial port to connect
  }
  DEBUG_SERIAL.println("Serial monitor connected!");
  
  // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(57600);
  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
}

void loop() {
  // put your main code here, to run repeatedly:
  FindServos();
  delay(5000);
}


DYNAMIXEL::InfoFromPing_t ping_info[32];
void FindServos(void) {
  Serial.println("\n----------------------------------------");
  Serial.println("Broadcasting ping to all servos...");
  Serial.flush(); // flush it as ping may take awhile... 
      
  if (uint8_t count_pinged = dxl.ping(DXL_BROADCAST_ID, ping_info, 
    sizeof(ping_info)/sizeof(ping_info[0]))) {
    Serial.println("Found servos:");
    Serial.println("----------------------------------------");
    Serial.println("ID\tModel\t\tFirmware");
    Serial.println("----------------------------------------");
    for (int i = 0; i < count_pinged; i++)
    {
      Serial.print(ping_info[i].id);
      Serial.print("\t");
      Serial.print(ping_info[i].model_number);
      Serial.print("\t\t");
      Serial.println(ping_info[i].firmware_version);
    }
    Serial.println("----------------------------------------");
  } else {
    Serial.println("No servos found!");
    Serial.print("Error code: ");
    Serial.println(dxl.getLastLibErrCode());
    Serial.println("----------------------------------------");
  }
}