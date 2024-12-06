#include "Nextion.h"

// Define Nextion components
NexNumber n0 = NexNumber(0, 6, "n1c"); // Page 0, Component ID 6, Name "n1c"
NexButton b0 = NexButton(0, 14, "b0"); // Page 0, Component ID 15, Name "b0"

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &b0,
    NULL
};

// Servo and current sensor parameters
const int T_DWELL = 150;
const int SERVO_WAIT_TIME = 1000;
const float vRef = 5.0;         // Arduino reference voltage
const float adcResolution = 1023.0; // 10-bit ADC resolution
const float vOffset = 2.5;      // Zero-current voltage (2.5V)
const float sensitivity = 0.04; // Sensitivity in V/A (40 mV/A for ACS758-050B)
const int currentSensePin = A0; // Current sensor connected to A0
const int currentThreshold = 1; // Failure threshold: 1A

// State machine states
enum State {
    IDLE,
    FORCE_REFRESH,
    GET_VALUE,
    SET_VALUE,
    SERVO_COMMAND,
    COMPLETE
};

// Substates for SERVO_COMMAND
enum ServoSubState {
    SERVO_START,
    SERVO_WAIT_TDWELL,
    SERVO_SEND_ZERO,
    SERVO_WAIT
};

State currentState = IDLE; // Initial state
ServoSubState servoSubState = SERVO_START; // Initial substate for servo commands
unsigned long lastTime = 0; // Timer for non-blocking delays
bool currentDetected = false; // Flag to track current detection

// Function to send data in the specified format
void sendData(byte data[], int length) {
    for (int i = 0; i < length; i++) {
        Serial1.write(data[i]); // Send data to the servo controller
    }
}

// Function to create and send a command for servo movement
void sendServoMoveCommand(byte servoID, int angle, int time) {
    byte data[10];

    // Map the angle. 0-1000 corresponds to 0-240 degrees.
    int controlAngle = map(angle, -120, 120, 0, 1000);

    // Header
    data[0] = 0x55;
    data[1] = 0x55;

    // Data Length
    data[2] = 0x08;

    // Command
    data[3] = 0x03; // CMD_SERVO_MOVE

    // Time (low and high byte)
    data[4] = 0x01;
    data[5] = time & 0xFF;
    data[6] = (time >> 8) & 0xFF;
    data[7] = servoID;  // ID
    // Angle (low and high byte)
    data[8] = controlAngle & 0xFF;        // Low byte of angle
    data[9] = (controlAngle >> 8) & 0xFF; // High byte of angle

    // Send the data
    sendData(data, 10);
}

// Function to read current and update the detection flag
void monitorCurrent() {
    int adcValue = analogRead(currentSensePin);
    float vInput = (adcValue / adcResolution) * vRef;
    float current = (vOffset - vInput) / sensitivity;

    if (current > currentThreshold) {
        currentDetected = true;
    }
}

// Non-blocking callback for button press event
void b0PopCallback(void *ptr) {
    if (currentState == IDLE) {
        currentState = FORCE_REFRESH;
        lastTime = millis();
    }
}

void handleStates() {
    switch (currentState) {
        case FORCE_REFRESH:
            if (millis() - lastTime >= 50) {
                currentState = GET_VALUE;
                lastTime = millis();
            }
            break;

        case GET_VALUE:
            if (millis() - lastTime >= 50) {
                currentState = SET_VALUE;
                lastTime = millis();
            }
            break;

        case SET_VALUE:
            if (millis() - lastTime >= 50) {
                currentState = SERVO_COMMAND;
                currentDetected = false; // Reset current detection flag
                lastTime = millis();
            }
            break;

        case SERVO_COMMAND:
            monitorCurrent(); // Continuously monitor current during this state
            switch (servoSubState) {
                case SERVO_START:
                    sendServoMoveCommand(0x01, 90, 100); // Actuate the key switch
                    servoSubState = SERVO_WAIT_TDWELL;
                    lastTime = millis();
                    break;

                case SERVO_WAIT_TDWELL:
                    if (millis() - lastTime >= T_DWELL) {
                        sendServoMoveCommand(0x01, 45, 100); // Reset the key switch
                        servoSubState = SERVO_WAIT;
                        lastTime = millis();
                    }
                    break;

                case SERVO_WAIT:
                    if (millis() - lastTime >= SERVO_WAIT_TIME) {
                        servoSubState = SERVO_START;
                        currentState = COMPLETE; // Finish the sequence
                    }
                    break;
            }
            break;

        case COMPLETE:
            if (!currentDetected) {
                Serial.println("Key switch failure detected!");
            } else {
                Serial.println("Operation complete, key switch functioning normally.");
            }
            currentState = IDLE; // Reset for the next button press
            break;
    }
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(9600);

    // Initialize Serial1 for Hiwonder Servo Controller
    Serial1.begin(9600);

    // nexSerial for Nextion defaults to Serial2
    nexSerial.begin(115200);
    delay(500);
    Serial.println("nexSerial initialized for Nextion");

    // Retry mechanism for Nextion initialization
    for (int i = 0; i < 3; i++) {
        if (nexInit()) {
            Serial.println("Nextion initialized successfully");
            break;
        }
        Serial.println("Nextion initialization failed, retrying...");
        delay(500); // Small delay before retrying
    }

    // Attach button callback
    b0.attachPop(b0PopCallback);
    Serial.println("Button callback attached");
}

void loop() {
    // Handle Nextion events
    nexLoop(nex_listen_list);

    // Manage the state machine
    handleStates();
}
