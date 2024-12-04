#include "Nextion.h"

// Define Nextion components
NexNumber n0 = NexNumber(0, 2, "n0"); // Page 0, Component ID 2, Name "n0"
NexButton b0 = NexButton(0, 3, "b0"); // Page 0, Component ID 3, Name "b0"
NexText t0 = NexText(0, 4, "t0");     // Page 0, Component ID 4, Name "t0"

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &b0,
    NULL
};

// Callback for button press event
void b0PopCallback(void *ptr) {
    if (!t0.setText("Button Pressed")) {
        // Optional: Handle failure
    }
    incrementNumber(n0);
}

bool incrementNumber(NexNumber &nexNumber) {
    uint32_t value = 0;

    // Get the current value
    if (nexNumber.getValue(&value)) {
        // Increment and set the new value
        return nexNumber.setValue(value + 1);
    }
    return false; // Return false if getting the value failed
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

    // Send a test message to Nextion
    if (!t0.setText("Ready")) {
        // Optional: Handle failure
    }
}

void loop() {
    nexLoop(nex_listen_list); // Handle Nextion events
}