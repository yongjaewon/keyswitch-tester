#include "Nextion.h"
#include "Hiwonder.h"

// Define Nextion components
NexNumber n0 = NexNumber(0, 2, "n0"); // Page 0, Component ID 2, Name "n0"
NexButton b0 = NexButton(0, 3, "b0"); // Page 0, Component ID 3, Name "b0"
NexText t0 = NexText(0, 4, "t0");     // Page 0, Component ID 4, Name "t0"

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &b0,
    NULL
};

// Instantiate the Hiwonder class for the servo controller
Hiwonder servoController(Serial2);

// Variable to track servo position state
bool servoPositionFlag = false;

// Function to convert degrees to the servo's control range (0–1000)
uint16_t degreesToServoValue(float degrees) {
    // Clamp degrees to the valid range of 0–240
    degrees = constrain(degrees, 0, 240);
    // Map degrees to the servo control range (0–1000)
    return map(degrees, 0, 240, 0, 1000);
}

// Callback for button press event
void b0PopCallback(void *ptr) {
    // Update number on the Nextion display
    uint32_t value = 0;
    if (n0.getValue(&value)) {
        n0.setValue(value + 1);
    }

    // Toggle between two servo angles: 90° and 180°
    uint16_t angle = servoPositionFlag ? 90 : 180;
    uint16_t position = degreesToServoValue(angle);
    servoController.move(1, position, 1000); // Move to the position
    servoPositionFlag = !servoPositionFlag; // Toggle the flag

    // Update text to reflect the current servo position
    String positionText = String("Servo Position: ") + angle;
    t0.setText(positionText.c_str());
}

void setup() {
    // Set up Serial1 for the Nextion display
    Serial1.setTX(12);
    Serial1.setRX(13);
    nexSerial = Serial1;
    nexSerial.begin(115200); // Hardcoded to 115200 baud rate

    // Initialize the Nextion display
    if (!nexInit()) {
        while (1); // Halt if Nextion initialization fails
    }

    // Attach button callback
    b0.attachPop(b0PopCallback);

    // Set initial message on Nextion display
    t0.setText("Ready");

    // Set up Serial2 for the Hiwonder servo controller
    // Serial2.setTX(24); // Set TX to GPIO24 --- causes a crash
    // Serial2.setRX(25); // Set RX to GPIO25 --- causes a crash
    // servoController.begin(9600); // Fixed at 9600 baud rate
}

void loop() {
    // Handle Nextion events
    nexLoop(nex_listen_list);
}