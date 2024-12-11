#include "Nextion.h"

// Define Nextion components
NexNumber n1t = NexNumber(0, 5, "n1t");  // #1 Target Count
NexNumber n1c = NexNumber(0, 6, "n1c");  // #1 Current Count
NexNumber n2t = NexNumber(0, 8, "n2t");  // #2 Target Count
NexNumber n2c = NexNumber(0, 9, "n2c");  // #2 Current Count
NexNumber n3t = NexNumber(0, 10, "n3t"); // #3 Target Count
NexNumber n3c = NexNumber(0, 11, "n3c"); // #3 Current Count
NexNumber n4t = NexNumber(0, 12, "n4t"); // #4 Target Count
NexNumber n4c = NexNumber(0, 13, "n4c"); // #4 Current Count

NexDSButton sw1 = NexDSButton(0, 7, "sw1"); // #1 On/Off Switch
NexDSButton sw2 = NexDSButton(0, 2, "sw2"); // #2 On/Off Switch
NexDSButton sw3 = NexDSButton(0, 3, "sw3"); // #3 On/Off Switch
NexDSButton sw4 = NexDSButton(0, 4, "sw4"); // #4 On/Off Switch

NexText t1s = NexText(0, 4, "t1s");  // #1 Starter Status
NexText t1k = NexText(0, 17, "t1k"); // #1 Key Switch Status
NexText t2s = NexText(0, 19, "t2s"); // #2 Starter Status
NexText t2k = NexText(0, 18, "t2k"); // #2 Key Switch Status
NexText t3s = NexText(0, 20, "t3s"); // #3 Starter Status
NexText t3k = NexText(0, 21, "t3k"); // #3 Key Switch Status
NexText t4s = NexText(0, 22, "t4s"); // #4 Starter Status
NexText t4k = NexText(0, 23, "t4k"); // #4 Key Switch Status

NexDSButton bt0 = NexDSButton(0, 24, "bt0"); // Master Enable/Disable
NexNumber n0 = NexNumber(0, 27, "n0");       // Actuation period in cycles per minute

// List of components to listen for events
NexTouch *nex_listen_list[] = {
    &sw1,
    &sw2,
    &sw3,
    &sw4,
    &bt0,
    NULL};

// Servo and current sensor parameters
const int T_DWELL = 600;
const int SERVO_WAIT_TIME = 1000;
const int zeroPoints[4] = {565, 525, 415, 595}; // Zero points for each station
const float vRef = 5.0;                         // Arduino reference voltage
const float adcResolution = 1023.0;             // 10-bit ADC resolution
const float vOffset = 2.5;                      // Zero-current voltage (2.5V)
const float sensitivity = 0.04;                 // Sensitivity in V/A (40 mV/A for ACS758-050B)
const int currentSensePin = A0;                 // Current sensor connected to A0
const int currentThreshold = 8;                 // Failure threshold: 8A

unsigned long actuationPeriodMillis = 10000; // Default actuation period in milliseconds
unsigned long lastPeriodUpdate = 0;          // Timer to track periodic updates from Nextion
bool masterEnable = false;                   // Master enable state

// State machine states
enum State
{
    IDLE,
    GET_VALUE,
    SET_VALUE,
    SERVO_COMMAND,
    COMPLETE
};

// Substates for SERVO_COMMAND
enum ServoSubState
{
    SERVO_START,
    SERVO_WAIT_TDWELL,
    SERVO_SEND_ZERO,
    SERVO_WAIT
};

State currentState = IDLE;                 // Initial state
ServoSubState servoSubState = SERVO_START; // Initial substate for servo commands

unsigned long lastActuationTime = 0; // Timer to track periodic actuation
unsigned long lastStateTime = 0;     // Timer for state transitions
bool currentDetected = false;        // Flag to track current detection

uint32_t enabledStations[4] = {0, 0, 0, 0}; // Store which stations are enabled
uint32_t numEnabledStations = 0;            // Count of enabled stations
uint32_t currentStationIndex = 0;           // Current station being processed

int failureCounts[4] = {0, 0, 0, 0};

// Function to send data in the specified format
void sendData(byte data[], int length)
{
    for (int i = 0; i < length; i++)
    {
        Serial1.write(data[i]); // Send data to the servo controller
    }
}

// Function to create and send a command for servo movement
void sendServoMoveCommand(byte servoID, int angle, int time)
{
    byte data[10];

    // Map the angle. 0-1000 corresponds to 0-240 degrees.
    int servoValue = zeroPoints[servoID - 1] + (25 / 6) * angle;

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
    data[7] = servoID; // ID
    // Angle (low and high byte)
    data[8] = servoValue & 0xFF;        // Low byte of angle
    data[9] = (servoValue >> 8) & 0xFF; // High byte of angle

    // Send the data
    sendData(data, 10);
}

void updateMasterEnable()
{
    uint32_t value;
    bool success = false; // Flag to track successful retrieval

    for (int attempt = 1; attempt <= 3; attempt++)
    {
        if (bt0.getValue(&value))
        {
            success = true;
            Serial.print("bt0 value retrieved successfully on attempt ");
            Serial.println(attempt);
            break; // Exit the loop on success
        }
        else
        {
            Serial.print("Attempt ");
            Serial.print(attempt);
            Serial.println(" failed to get value from bt0.");
        }
    }

    if (success)
    {
        masterEnable = (value == 0); // Enabled if bt0 value is 0
        if (!masterEnable)
        {
            Serial.println("Master enable is OFF. Stopping actuations.");
        }
        else
        {
            Serial.println("Master enable is ON. Resuming actuations.");
        }
    }
    else
    {
        Serial.println("Failed to retrieve bt0 value after 3 attempts.");
        // Handle the failure as needed
        // Example: Retain previous state or set a default value
        // masterEnable = false; // Example default
    }
}

// Function to read current and update the detection flag
void monitorCurrent()
{
    int adcValue = analogRead(currentSensePin);
    float vInput = (adcValue / adcResolution) * vRef;
    float current = (vOffset - vInput) / sensitivity;

    if (current > currentThreshold)
    {
        currentDetected = true;
    }
}

// Update enabled stations
void updateEnabledStations()
{
    uint32_t value;
    numEnabledStations = 0; // Reset count

    // Check each switch and update enabledStations array
    if (sw1.getValue(&value) && value == 1)
        enabledStations[numEnabledStations++] = 1;
    if (sw2.getValue(&value) && value == 1)
        enabledStations[numEnabledStations++] = 2;
    if (sw3.getValue(&value) && value == 1)
        enabledStations[numEnabledStations++] = 3;
    if (sw4.getValue(&value) && value == 1)
        enabledStations[numEnabledStations++] = 4;

    if (numEnabledStations == 0)
    {
        Serial.println("No stations enabled.");
    }
}

void handleStates()
{
    static uint32_t currentCount = 0;                  // Track the current count locally
    static unsigned long lastActuationTime = millis(); // Track time for periodic actuation

    // Calculate delay between stations based on enabled stations
    unsigned long stationDelay = actuationPeriodMillis / max(1, numEnabledStations);

    switch (currentState)
    {
    case IDLE:
        if (millis() - lastActuationTime >= stationDelay)
        {
            lastActuationTime = millis();
            updateEnabledStations();
            if (numEnabledStations > 0)
            {
                currentStationIndex = (currentStationIndex + 1) % numEnabledStations;
                Serial.print("Processing station: ");
                Serial.println(enabledStations[currentStationIndex]);
                currentState = GET_VALUE;
            }
        }
        break;

    case GET_VALUE:
        if (millis() - lastStateTime >= 50)
        {
            // Retrieve and update current count for the station
            NexNumber *currentCountComponent;
            if (enabledStations[currentStationIndex] == 1)
                currentCountComponent = &n1c;
            else if (enabledStations[currentStationIndex] == 2)
                currentCountComponent = &n2c;
            else if (enabledStations[currentStationIndex] == 3)
                currentCountComponent = &n3c;
            else
                currentCountComponent = &n4c;

            if (currentCountComponent->getValue(&currentCount))
            {
                Serial.print("Current count retrieved for station ");
                Serial.print(enabledStations[currentStationIndex]);
                Serial.print(": ");
                Serial.println(currentCount);
                currentState = SET_VALUE;
            }
            else
            {
                Serial.println("Failed to get value. Retrying...");
            }
            lastStateTime = millis();
        }
        break;

    case SET_VALUE:
        if (millis() - lastStateTime >= 50)
        {
            currentCount++;
            NexNumber *currentCountComponent;
            if (enabledStations[currentStationIndex] == 1)
                currentCountComponent = &n1c;
            else if (enabledStations[currentStationIndex] == 2)
                currentCountComponent = &n2c;
            else if (enabledStations[currentStationIndex] == 3)
                currentCountComponent = &n3c;
            else
                currentCountComponent = &n4c;

            if (currentCountComponent->setValue(currentCount))
            {
                Serial.print("Updated count to: ");
                Serial.println(currentCount);
                currentState = SERVO_COMMAND;
                servoSubState = SERVO_START;
                currentDetected = false;
            }
            else
            {
                Serial.println("Failed to update value. Retrying...");
            }
            lastStateTime = millis();
        }
        break;

    case SERVO_COMMAND:
        monitorCurrent(); // Continuously monitor current during this state
        switch (servoSubState)
        {
        case SERVO_START:
            sendServoMoveCommand(enabledStations[currentStationIndex], 105, 200); // Actuate the current station's key switch
            servoSubState = SERVO_WAIT_TDWELL;
            lastStateTime = millis();
            break;

        case SERVO_WAIT_TDWELL:
            if (millis() - lastStateTime >= T_DWELL)
            {
                sendServoMoveCommand(enabledStations[currentStationIndex], 0, 100); // Reset the current station's key switch
                servoSubState = SERVO_WAIT;
                lastStateTime = millis();
            }
            break;

        case SERVO_WAIT:
            if (millis() - lastStateTime >= SERVO_WAIT_TIME)
            {
                servoSubState = SERVO_START;
                currentState = COMPLETE; // Finish the sequence
            }
            break;
        }
        break;

    case COMPLETE:
        NexText *currentTextComponent;
        NexDSButton *currentSwitchComponent;

        // Determine the appropriate components for the current station
        if (enabledStations[currentStationIndex] == 1)
        {
            currentTextComponent = &t1k;
            currentSwitchComponent = &sw1;
        }
        else if (enabledStations[currentStationIndex] == 2)
        {
            currentTextComponent = &t2k;
            currentSwitchComponent = &sw2;
        }
        else if (enabledStations[currentStationIndex] == 3)
        {
            currentTextComponent = &t3k;
            currentSwitchComponent = &sw3;
        }
        else
        {
            currentTextComponent = &t4k;
            currentSwitchComponent = &sw4;
        }

        // Perform the actions on the determined components
        if (!currentDetected)
        {
            uint32_t failedStation = enabledStations[currentStationIndex];
            failureCounts[failedStation - 1]++;
            char buffer[50];
            sprintf(buffer, "%s%d", "Failure Attept ", failureCounts[failedStation - 1]);
            Serial.println(buffer);
            Serial.println("Key switch failure detected!");
            currentTextComponent->setText(buffer);
            currentTextComponent->Set_background_color_bco(64768); // Orange

            if (failureCounts[failedStation - 1] >= 4)
            {
                Serial.println("Key switch failure detected (4 in a row)!");
                currentTextComponent->setText("Key Switch Failed");
                currentTextComponent->Set_background_color_bco(63488); // Red
                currentSwitchComponent->setValue(0);

                // Update enabled stations after failure
                updateEnabledStations();

                // Find the new index of the current station in the updated list
                for (uint32_t i = 0; i < numEnabledStations; i++)
                {
                    if (enabledStations[i] == failedStation)
                    {
                        currentStationIndex = i;
                        break;
                    }
                }
                if (currentStationIndex >= numEnabledStations)
                {
                    currentStationIndex = 0; // If station not found, reset to first
                }
                // Skip delay after failure
                lastActuationTime = millis();
            }
            else
            {
                // Not yet a total failure, just a warning or do nothing
                Serial.println("Key switch did not detect current, but not failing yet.");
            }
        }
        else
        {
            // Reset failure count if successful
            failureCounts[failedStation - 1] = 0;
            Serial.println("Operation complete, key switch functioning normally.");
            currentTextComponent->setText("Key Switch Normal");
            currentTextComponent->Set_background_color_bco(1024); // Green
        }

        // Update enabled stations and adjust currentStationIndex
        updateEnabledStations();
        updateActuationPeriod();

        currentState = IDLE; // Reset for the next station
        break;

void updateActuationPeriod()
{
    uint32_t cyclesPerMinute;
    bool success = false;

    for (int attempt = 0; attempt < 3; attempt++)
    {
        if (n0.getValue(&cyclesPerMinute))
        {
            success = true;
            break; // Exit the loop on successful retrieval
        }
        Serial.print("Attempt ");
        Serial.print(attempt + 1);
        Serial.println(" failed to fetch cycles per minute from Nextion.");
    }

    if (success)
    {
        // Convert cycles per minute to milliseconds per cycle
        if (cyclesPerMinute > 0)
        {
            actuationPeriodMillis = 60000 / cyclesPerMinute;
            Serial.print("Updated actuation period (ms): ");
            Serial.println(actuationPeriodMillis);
        }
        else
        {
            Serial.println("Invalid cycles per minute from Nextion.");
        }
    }
    else
    {
        Serial.println("Failed to fetch cycles per minute from Nextion after 3 attempts.");
    }
}

void setup()
{
    // Initialize Serial for debugging
    Serial.begin(9600);

    // Initialize Serial1 for Hiwonder Servo Controller
    Serial1.begin(9600);

    // nexSerial for Nextion defaults to Serial2
    nexSerial.begin(115200);
    delay(500);
    Serial.println("nexSerial initialized for Nextion");

    // Retry mechanism for Nextion initialization
    for (int i = 0; i < 3; i++)
    {
        if (nexInit())
        {
            Serial.println("Nextion initialized successfully");
            break;
        }
        Serial.println("Nextion initialization failed, retrying...");
        delay(500); // Small delay before retrying
    }
}

void loop()
{
    // Handle Nextion events
    nexLoop(nex_listen_list);

    // Manage the state machine
    handleStates();
}