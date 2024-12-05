#include "Nextion.h"

// Define Nextion components
NexNumber n1c = NexNumber(0, 6, "n1c"); // Page 0, Component ID 2, Name "n0"
NexButton b0 = NexButton(0, 15, "b0"); // Page 0, Component ID 3, Name "b0"
NexText t0 = NexText(0, 4, "t0");     // Page 0, Component ID 4, Name "t0"

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &b0,
    NULL
};

// Servo parameters
const int T_DWELL = 150;
const int SERVO_WAIT_TIME = 1000;

// State machine states
enum State {
    IDLE,
    FORCE_REFRESH,
    GET_VALUE,
    SET_VALUE,
    SET_TEXT,
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
const unsigned long retryDelay = 50; // Delay between retries (ms)

uint32_t value = 0; // Holds the current value of n0
int retryCount = 0; // Retry counter
const int maxRetries = 3; // Maximum retries

// Function to send data in the specified format
void sendData(byte data[], int length) {
    Serial.println("Sending data:");
    for (int i = 0; i < length; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
        Serial1.write(data[i]); // Use Serial1 for Hiwonder
    }
    Serial.println();
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
    data[4] = time & 0xFF;        // Low byte of time
    data[5] = (time >> 8) & 0xFF; // High byte of time

    // Servo ID
    data[6] = servoID;

    // Angle (low and high byte)
    data[7] = controlAngle & 0xFF;        // Low byte of angle
    data[8] = (controlAngle >> 8) & 0xFF; // High byte of angle

    // Checksum
    uint16_t checksum = 0;
    for (int i = 2; i < 9; i++) {
        checksum += data[i];
    }
    data[9] = (~checksum) & 0xFF; // Bitwise NOT of checksum

    // Send the data
    sendData(data, 10);
    Serial.println("Servo move command sent.");
}

// Non-blocking callback for button press event
void b0PopCallback(void *ptr) {
    if (currentState == IDLE) {
        Serial.println("Button pressed!");
        currentState = FORCE_REFRESH;
        lastTime = millis();
    }
}

void handleStates() {
    switch (currentState) {
        case FORCE_REFRESH:
            if (millis() - lastTime >= retryDelay) {
                // Force refresh of n0
                nexSerial.print("ref n0");
                nexSerial.write(0xFF);
                nexSerial.write(0xFF);
                nexSerial.write(0xFF);
                Serial.println("Forced refresh of n0.");
                currentState = GET_VALUE;
                retryCount = 0;
                lastTime = millis();
            }
            break;

        case GET_VALUE:
            if (millis() - lastTime >= retryDelay) {
                if (n0.getValue(&value)) {
                    Serial.print("Current value: ");
                    Serial.println(value);
                    currentState = SET_VALUE;
                    retryCount = 0;
                } else {
                    Serial.print("Failed to get n0 value. Retrying... Attempt ");
                    Serial.println(retryCount + 1);
                    retryCount++;
                    if (retryCount >= maxRetries) {
                        Serial.println("Failed to get n0 value after retries. Aborting.");
                        currentState = COMPLETE;
                    }
                }
                lastTime = millis();
            }
            break;

        case SET_VALUE:
            if (millis() - lastTime >= retryDelay) {
                value += 1;
                if (n0.setValue(value)) {
                    Serial.print("Updated value to: ");
                    Serial.println(value);
                    currentState = SET_TEXT;
                    retryCount = 0;
                } else {
                    Serial.print("Failed to update n0 value. Retrying... Attempt ");
                    Serial.println(retryCount + 1);
                    retryCount++;
                    if (retryCount >= maxRetries) {
                        Serial.println("Failed to update n0 value after retries. Aborting.");
                        currentState = COMPLETE;
                    }
                }
                lastTime = millis();
            }
            break;

        case SET_TEXT:
            if (millis() - lastTime >= retryDelay) {
                if (t0.setText("Button Pressed")) {
                    Serial.println("Text updated successfully.");
                    currentState = SERVO_COMMAND;
                    retryCount = 0;
                } else {
                    Serial.print("Failed to update text. Retrying... Attempt ");
                    Serial.println(retryCount + 1);
                    retryCount++;
                    if (retryCount >= maxRetries) {
                        Serial.println("Failed to update text after retries. Aborting.");
                        currentState = COMPLETE;
                    }
                }
                lastTime = millis();
            }
            break;

        case SERVO_COMMAND:
            switch (servoSubState) {
                case SERVO_START:
                    Serial.println("Sending initial servo command...");
                    sendServoMoveCommand(0x01, 45, 100); // Move servo to 45 degrees
                    servoSubState = SERVO_WAIT_TDWELL;
                    lastTime = millis();
                    break;

                case SERVO_WAIT_TDWELL:
                    if (millis() - lastTime >= T_DWELL) {
                        Serial.println("Sending servo to 0 degrees...");
                        sendServoMoveCommand(0x01, 0, 100); // Move servo to 0 degrees
                        servoSubState = SERVO_WAIT_1000;
                        lastTime = millis();
                    }
                    break;

                case SERVO_WAIT:
                    if (millis() - lastTime >= SERVO_WAIT_TIME) {
                        Serial.println("Servo sequence complete.");
                        currentState = COMPLETE; // Finish the sequence
                    }
                    break;
            }
            break;

        case COMPLETE:
            Serial.println("Operation complete.");
            currentState = IDLE; // Reset for the next button press
            break;
    }
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(9600);
    Serial.println("Initializing...");

    // Initialize Serial1 for Hiwonder Servo Controller
    Serial1.begin(9600);
    Serial.println("Serial1 initialized for Hiwonder Servo Controller");

    // nexSerial for Nextion defaults to Serial2
    nexSerial.begin(115200);
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

    // Set initial message on Nextion display
    Serial.println("Setting initial text to 'Ready'");
    t0.setText("Ready");
}

void loop() {
    // Handle Nextion events
    nexLoop(nex_listen_list);

    // Manage the state machine
    handleStates();
}
