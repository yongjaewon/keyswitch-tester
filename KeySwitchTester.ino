#include "Nextion.h"

// Define Nextion components
NexButton b0 = NexButton(0, 3, "b0"); // Page 0, Component ID 3, Name "b0"
NexText t0 = NexText(0, 4, "t0");     // Page 0, Component ID 4, Name "t0"

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &b0,
    NULL
};

// Callback for button press event
void b0PopCallback(void *ptr) {
    Serial.println("Button b0 pressed!");
    t0.setText("Button Pressed"); // Update text field on the Nextion display
}

void setup() {
    // Initialize USB Serial for debugging
    Serial.begin(9600);
    delay(500); // Reduce delay to 500ms
    Serial.println("Initializing...");

    // Set up Serial1 for the Nextion display
    Serial1.setTX(12); // Set TX to GPIO12
    Serial1.setRX(13); // Set RX to GPIO13
    Serial1.begin(9600);
    Serial.println("Serial1 initialized.");

    // Configure the library to use Serial1
    nexSerial = Serial1; // Assign Serial1 to nexSerial

    // Initialize the Nextion display
    if (!nexInit()) {
        Serial.println("Failed to initialize Nextion!");
        while (1); // Stop if Nextion initialization fails
    }
    Serial.println("Nextion initialized.");

    // Attach button callback
    b0.attachPop(b0PopCallback);

    // Send a test message to Nextion
    t0.setText("Ready");
    Serial.println("Sent initial message to Nextion.");
}

void loop() {
    nexLoop(nex_listen_list);
}