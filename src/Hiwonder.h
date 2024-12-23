// Hiwonder.h

#ifndef HIWONDER_H
#define HIWONDER_H

#include <Arduino.h>

// Macro Definitions
#define GET_LOW_BYTE(A)  (uint8_t)((A) & 0xFF)
#define GET_HIGH_BYTE(A) (uint8_t)(((A) >> 8) & 0xFF)
#define BYTE_TO_HW(A, B) ((((uint16_t)(A)) << 8) | (uint8_t)(B))

// Servo Frame Definitions
#define FRAME_HEADER                0x55
#define MOVE_TIME_WRITE             1
#define MOVE_TIME_READ              2
#define MOVE_TIME_WAIT_WRITE        7
#define MOVE_TIME_WAIT_READ         8
#define MOVE_START                  11
#define MOVE_STOP                   12
#define ID_WRITE                    13
#define ID_READ                     14
#define ANGLE_OFFSET_ADJUST         17
#define ANGLE_OFFSET_WRITE          18
#define ANGLE_OFFSET_READ           19
#define ANGLE_LIMIT_WRITE           20
#define ANGLE_LIMIT_READ            21
#define VIN_LIMIT_WRITE             22
#define VIN_LIMIT_READ              23
#define TEMP_MAX_LIMIT_WRITE        24
#define TEMP_MAX_LIMIT_READ         25
#define TEMP_READ                   26
#define VIN_READ                    27
#define POS_READ                    28
#define SERVO_OR_MOTOR_MODE_WRITE   29
#define SERVO_OR_MOTOR_MODE_READ    30
#define LOAD_OR_UNLOAD_WRITE        31
#define LOAD_OR_UNLOAD_READ         32
#define LED_CTRL_WRITE              33
#define LED_CTRL_READ               34
#define LED_ERROR_WRITE             35
#define LED_ERROR_READ              36

class Hiwonder
{
    public:
        // Constructor: Takes a HardwareSerial object as an argument
        Hiwonder(HardwareSerial &serialPort);

        // Servo Control Methods
        void move(uint8_t id, int16_t position, uint16_t time);
        void stop(uint8_t id);
        void setID(uint8_t oldID, uint8_t newID);
        void setMode(uint8_t id, uint8_t mode, int16_t speed);
        void load(uint8_t id);
        void unload(uint8_t id);

        // Servo Read Methods
        int getPosition(uint8_t id);
        int getVin(uint8_t id);

    private:
        HardwareSerial &SerialX;
        byte checkSum(const byte buf[], size_t length);
        int receiveHandle(byte *ret);
};

#endif // HIWONDER_H