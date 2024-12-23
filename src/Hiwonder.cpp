#include "Hiwonder.h"

//#define HIWONDER_DEBUG

Hiwonder::Hiwonder(HardwareSerial &serialPort) : SerialX(serialPort) {
}

// Checksum Calculation
byte Hiwonder::checkSum(byte buf[])
{
  byte i;
  uint16_t temp = 0;
  for (i = 2; i < buf[3] + 2; i++) {
    temp += buf[i];
  }
  temp = ~temp;
  return (byte)temp;
}

void Hiwonder::move(uint8_t id, int16_t position, uint16_t time)
{
  byte buf[10];
  
  // Clamp position to [0, 1000]
  if (position < 0) position = 0;
  if (position > 1000) position = 1000;
  
  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 7;
  buf[4] = MOVE_TIME_WRITE;
  buf[5] = GET_LOW_BYTE(position);
  buf[6] = GET_HIGH_BYTE(position);
  buf[7] = GET_LOW_BYTE(time);
  buf[8] = GET_HIGH_BYTE(time);
  buf[9] = checkSum(buf);

  SerialX.write(buf, 10);
}

void Hiwonder::stop(uint8_t id)
{
  byte buf[6];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 3;
  buf[4] = MOVE_STOP;
  buf[5] = checkSum(buf);

  SerialX.write(buf, 6);
}

void Hiwonder::setID(uint8_t oldID, uint8_t newID)
{
  byte buf[7];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = oldID;
  buf[3] = 4;
  buf[4] = ID_WRITE;
  buf[5] = newID;
  buf[6] = checkSum(buf);

  SerialX.write(buf, 7);
  
#ifdef HIWONDER_DEBUG
  Serial.println("SERVO ID WRITE");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif
}

void Hiwonder::setMode(uint8_t id, uint8_t mode, int16_t speed)
{
  byte buf[10];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 7;
  buf[4] = SERVO_OR_MOTOR_MODE_WRITE;
  buf[5] = mode;
  buf[6] = 0;
  buf[7] = GET_LOW_BYTE((uint16_t)speed);
  buf[8] = GET_HIGH_BYTE((uint16_t)speed);
  buf[9] = checkSum(buf);

  SerialX.write(buf, 10);

#ifdef HIWONDER_DEBUG
  Serial.println("SERVO SET MODE");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif
}

void Hiwonder::load(uint8_t id)
{
  byte buf[7];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 4;
  buf[4] = LOAD_OR_UNLOAD_WRITE;
  buf[5] = 1;
  buf[6] = checkSum(buf);
  
  SerialX.write(buf, 7);
  
#ifdef HIWONDER_DEBUG
  Serial.println("SERVO LOAD WRITE");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif
}

void Hiwonder::unload(uint8_t id)
{
  byte buf[7];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 4;
  buf[4] = LOAD_OR_UNLOAD_WRITE;
  buf[5] = 0;
  buf[6] = checkSum(buf);
  
  SerialX.write(buf, 7);
  
#ifdef HIWONDER_DEBUG
  Serial.println("SERVO UNLOAD WRITE");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif
}


int Hiwonder::receiveHandle(byte *ret)
{
  bool frameStarted = false;
  bool receiveFinished = false;
  byte frameCount = 0;
  byte dataCount = 0;
  byte dataLength = 2;
  byte rxBuf;
  byte recvBuf[32];
  byte i;

  while (SerialX.available()) {
    rxBuf = SerialX.read();
    delayMicroseconds(100);
    if (!frameStarted) {
      if (rxBuf == FRAME_HEADER) {
        frameCount++;
        if (frameCount == 2) {
          frameCount = 0;
          frameStarted = true;
          dataCount = 1;
        }
      }
      else {
        frameStarted = false;
        dataCount = 0;
        frameCount = 0;
      }
    }
    if (frameStarted) {
      recvBuf[dataCount] = (uint8_t)rxBuf;
      if (dataCount == 3) {
        dataLength = recvBuf[dataCount];
        if (dataLength < 3 || dataCount > 7) {
          dataLength = 2;
          frameStarted = false;
        }
      }
      dataCount++;
      if (dataCount == dataLength + 3) {
        
#ifdef HIWONDER_DEBUG
        Serial.print("RECEIVE DATA: ");
        for (i = 0; i < dataCount; i++) {
          Serial.print(recvBuf[i], HEX);
          Serial.print(":");
        }
        Serial.println(" ");
#endif

        if (checkSum(recvBuf) == recvBuf[dataCount - 1]) {
          
#ifdef HIWONDER_DEBUG
          Serial.println("Check SUM OK!!");
          Serial.println("");
#endif

          frameStarted = false;
          memcpy(ret, recvBuf + 4, dataLength);
          return 1;
        }
        return -1;
      }
    }
  }
}


int getPosition(uint8_t id)
{
  int count = 10000;
  int ret;
  byte buf[6];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 3;
  buf[4] = POS_READ;
  buf[5] = checkSum(buf);

#ifdef HIWONDER_DEBUG
  Serial.println("LOBOT SERVO Pos READ");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif

  while (SerialX.available())
    SerialX.read();

  SerialX.write(buf, 6);

  while (!SerialX.available()) {
    count -= 1;
    if (count < 0)
      return -2048;
  }

  if (receiveHandle(buf) > 0)
    ret = (int16_t)BYTE_TO_HW(buf[2], buf[1]);
  else
    ret = -2048;

#ifdef HIWONDER_DEBUG
  Serial.println(ret);
#endif
  return ret;
}

int getVin(uint8_t id)
{
  int count = 10000;
  int ret;
  byte buf[6];

  buf[0] = buf[1] = FRAME_HEADER;
  buf[2] = id;
  buf[3] = 3;
  buf[4] = VIN_READ;
  buf[5] = checkSum(buf);

#ifdef HIWONDER_DEBUG
  Serial.println("VIN READ");
  int debug_value_i = 0;
  for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++)
  {
    Serial.print(buf[debug_value_i], HEX);
    Serial.print(":");
  }
  Serial.println(" ");
#endif

  while (SerialX.available())
    SerialX.read();

  SerialX.write(buf, 6);

  while (!SerialX.available()) {
    count -= 1;
    if (count < 0)
      return -2048;
  }

  if (receiveHandle(buf) > 0)
    ret = (int16_t)BYTE_TO_HW(buf[2], buf[1]);
  else
    ret = -2049;

#ifdef HIWONDER_DEBUG
  Serial.println(ret);
#endif
  return ret;
}