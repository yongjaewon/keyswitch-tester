#include "Nextion.h"

// Define Nextion components

NexNumber nTarget[] =
{
    NexNumber(1, 1, "nT0"),
    NexNumber(1, 3, "nT1"),
    NexNumber(1, 5, "nT2"),
    NexNumber(1, 7, "nT3")
};

NexNumber nCounter[] =
{
    NexNumber(1, 2, "nC0"),
    NexNumber(1, 4, "nC1"),
    NexNumber(1, 6, "nC2"),
    NexNumber(1, 8, "nC3")
};

NexCrop motorState[] =
{
    NexCrop(1, 10, "crM0"),
    NexCrop(1, 12, "crM1"),
    NexCrop(1, 14, "crM2"),
    NexCrop(1, 16, "crM3")
};

NexCrop keyState[] =
{
    NexCrop(1, 11, "crK0"),
    NexCrop(1, 13, "crK1"),
    NexCrop(1, 15, "crK2"),
    NexCrop(1, 17, "crK3")
};

NexNumber nMotorFails[] =
{
    NexNumber(1, 20, "nMF0"),
    NexNumber(1, 21, "nMF1"),
    NexNumber(1, 22, "nMF2"),
    NexNumber(1, 23, "nMF3")
};

NexNumber nKeyFails[] =
{
    NexNumber(1, 24, "nKF0"),
    NexNumber(1, 25, "nKF1"),
    NexNumber(1, 26, "nKF2"),
    NexNumber(1, 27, "nKF3")
};

NexDSButton stationEnable[] =
{
    NexDSButton(1, 28, "dsS0"),
    NexDSButton(1, 29, "dsS1"),
    NexDSButton(1, 30, "dsS2"),
    NexDSButton(1, 31, "dsS3")
};

NexNumber keyAmps[] =
{
    NexNumber(1, 35, "fAmps0"),
    NexNumber(1, 36, "fAmps1"),
    NexNumber(1, 37, "fAmps2"),
    NexNumber(1, 38, "fAmps3")
};

NexButton bReset[] =
{
    NexButton(2, 3, "b1"),
    NexButton(3, 3, "b1"),
    NexButton(4, 3, "b1"),
    NexButton(5, 3, "b1")
};

NexPage activePage = NexPage(1, 0, "active");
NexDSButton dsEnable = NexDSButton(1, 9, "dsEnable");
NexCrop sdState = NexCrop(1, 18, "crSD");
NexText msg = NexText(1, 32, "tMsg");
NexCrop battState = NexCrop(1, 33, "crVBatt");
NexNumber vBatt = NexNumber(1, 34, "fVBatt");
NexNumber cpm = NexNumber(1, 39, "nCPM");

// List of components to listen for events
NexTouch *nex_listen_list[] =
{
    &bReset[0],
    &bReset[1],
    &bReset[2],
    &bReset[3],
    NULL
};

// Servo and current sensor parameters
const uint16_t T_DWELL = 600;
const uint16_t SERVO_WAIT_TIME = 1000;
const uint16_t ZERO_POINT[4] = {565, 525, 4125, 595}; // Zero points for each station
const float V_REF = 5.0;                        // Arduino reference voltage
const float ADC_RESOLUTION = 1023.0;            // 10-bit ADC resolution
const float V_OFFSET = 2.5;                     // Zero-current voltage (2.5V)
const float SENSITIVITY = 0.04;                 // Sensitivity in V/A (40 mV/A for ACS758-050B)
const uint8_t AMPS_PIN = A0;                        // Current sensor connected to A0
const uint8_t AMPS_THRESHOLD = 5;                   // Failure threshold: 5A
const uint8_t FAILURE_THRESHOLD = 10;                 // Total number of failures threshold: 10
const uint8_t AMPS_N_TOP = 20;                      // Number of top readings to maintain
const uint8_t STATE_DELAY = 50;

uint16_t stationDelay = 10000;             // Time delay between stations
unsigned long tlastCycle = 0;                   // Timer to track periodic updates from Nextion
bool masterEnable = false;                      // Master enable state

float topReadings[AMPS_N_TOP];                  // Array to store top N readings
uint8_t topCount = 0;                               // Current number of readings in topReadings

// Display state colors
enum Color {
  GRAY = 0,
  GREEN = 1,
  ORANGE = 2,
  RED = 3
};

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

bool enabledStations[4] = {0, 0, 0, 0};     // Store which stations are enabled
uint8_t numEnabledStations = 0;             // Count of enabled stations
uint8_t currentStationIndex = 0;               // Current station being processed

int failureCounts[4] = {0, 0, 0, 0};

void insertTopReading(float current)
{
    // If we haven't filled the topReadings array yet
    if (topCount < AMPS_N_TOP)
    {
        topReadings[topCount] = current;
        topCount++;
        
        // Sort the array in descending order
        for(int i = topCount - 1; i > 0; i--)
        {
            if(topReadings[i] > topReadings[i - 1])
            {
                float temp = topReadings[i];
                topReadings[i] = topReadings[i - 1];
                topReadings[i - 1] = temp;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        // Compare with the smallest (last element)
        if(current > topReadings[AMPS_N_TOP - 1])
        {
            topReadings[AMPS_N_TOP - 1] = current;
            
            // Move the new reading up to maintain descending order
            for(int i = AMPS_N_TOP - 1; i > 0; i--)
            {
                if(topReadings[i] > topReadings[i - 1])
                {
                    float temp = topReadings[i];
                    topReadings[i] = topReadings[i - 1];
                    topReadings[i - 1] = temp;
                }
                else
                {
                    break;
                }
            }
        }
    }
}

// Function to send data in the specified format
void sendData(uint8_t data[], int length)
{
    for (int i = 0; i < length; i++)
    {
        Serial1.write(data[i]); // Send data to the servo controller
    }
}

// Function to create and send a command for servo movement
void sendServoMoveCommand(uint8_t servoID, int angle, int time)
{
    uint8_t data[10];

    // Map the angle. 0-1000 corresponds to 0-240 degrees.
    int servoValue = ZERO_POINT[servoID - 1] + (25 / 6) * angle;

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
    uint32_t isEnabled;
    bool success = false; // Flag to track successful retrieval

    for (int attempt = 1; attempt <= 3; attempt++)
    {
        if (dsEnable.getValue(&isEnabled))
        {
            success = true;
            break; // Exit the loop on success
        }
    }

    if (success)
    {
        if (isEnabled)
        {
            masterEnable = true;
            Serial.println("Master enable is ON.");
        }
        else
        {
            masterEnable = false;
            Serial.println("Master enable is OFF.");
        }
    }
    else
    {
        Serial.println("Failed to retrieve masterEnable value after 3 attempts.");
    }
}

// Function to read current and update the top readings
void monitorCurrent()
{
    uint16_t adcValue = analogRead(AMPS_PIN);
    float vInput = (adcValue / ADC_RESOLUTION) * V_REF;
    float current = (V_OFFSET - vInput) / SENSITIVITY;

    insertTopReading(current);
}

uint16_t calculatePeakAmps()
{
    // If there are no readings, return 0
    if (topCount == 0)
    {
        return 0;
    }
    
    // Calculate the sum of the top readings
    float sum = 0.0f;
    for (int i = 0; i < topCount; i++)
    {
        sum += topReadings[i];
    }
    float average = sum / topCount;
    
    // Multiply by 10 to shift decimal, add 0.5 for rounding, and cast to int
    return (int)(average * 10.0f + 0.5f); // Returns the average value as an integer in tenth of an Amp
}

// Update enabled stations
void updateEnabledStations()
{
    numEnabledStations = 0; // Reset count

    // Iterate through each station
    for (uint8_t i = 0; i < 4; i++)
    {
        uint32_t isEnabled = 0;
        bool success = 0;

        // Attempt to get the value up to 3 times
        for (uint8_t attempt = 0; attempt < 3; attempt++)
        {
            if (stationEnable[i].getValue(&isEnabled))
            {
                success = true;
                break;
            }
        }

        if (success) enabledStations[i] = isEnabled;
    }
}

void calculateAmps()
{
    // Perform the actions on the determined components
    uint16_t avgCurrent() = calculatePeakAmps();
    for (uint8_t i = 0; i < 3, i++)
    {
      if (keyAmps[currentStationIndex].setValue(avgCurrent)) break;
    }

    if (avgCurrent < 50) // If less than 5.0 Amps
    {
        uint8_t currentFails = ++failureCounts[currentStationIndex];
        failureCounts[currentStationIndex]++;
        for (uint8_t i = 0; i < 4; i++)
        {
            if (nKeyFails[currentStationIndex].setValue(currentFails)) break;
        }

        if (failureCounts[currentStationIndex] > FAILURE_THRESHOLD)
        {
           for (uint8_t i = 0; i < 4; i++)
           {
                if (keyState[currentStationIndex].setPic(RED)) break;
           }
        }
    }
    resetAmps();
    currentState = IDLE; // Reset for the next station
}

void resetAmps()
{
    for (int i = 0; i < AMPS_N_TOP; i++)
    {
        topReadings[i] = 0.0f;
    }
    topCount = 0;
}

void handleStates()
{
    static uint32_t currentCount = 0;   // Track the current count locally

    switch (currentState)
    {
    case IDLE:
        updateEnabledStations();
        updateStationDelay();
        if (millis() - lastActuationTime >= stationDelay)
        {
            if (numEnabledStations > 0)
            {
                for(int i = 0; i < 4; i++)
                {
                    if(enabledStations[currentStationIndex]) break;
                    currentStationIndex = (currentStationIndex + 1) % 4;
                }
                currentState = GET_VALUE;
            }
            lastStateTime = millis();
        }
        break;

    case GET_VALUE:
        if (millis() - lastStateTime >= STATE_DELAY)
        {
            if (nCounter[currentStationIndex].getValue(&currentCount))
            {
                currentState = SET_VALUE;
            }
            lastStateTime = millis();
        }
        break;

    case SET_VALUE:
        if (millis() - lastStateTime >= STATE_DELAY)
        {
            currentCount++;
            if (nCounter[currentStationIndex].setValue(currentCount))
            {
                currentState = SERVO_COMMAND;
                servoSubState = SERVO_START;
            }
            lastStateTime = millis();
        }
        break;

    case SERVO_COMMAND:
        monitorCurrent(); // Continuously monitor current during this state
        switch (servoSubState)
        {
        case SERVO_START:
            sendServoMoveCommand(currentStationIndex + 1, 105, 200); // Actuate the current station's key switch
            servoSubState = SERVO_WAIT_TDWELL;
            lastStateTime = millis();
            break;

        case SERVO_WAIT_TDWELL:
            if (millis() - lastStateTime >= T_DWELL)
            {
                sendServoMoveCommand(currentStationIndex + 1, 0, 100); // Reset the current station's key switch
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
        cleanUp();
        break;
    }
}

void updateStationDelay()
{
    bool success = false;
    uint32_t speed = 1;

    for (int attempt = 0; attempt < 3; attempt++)
    {
        if (cpm.getValue(&speed))
        {
            success = true;
            break;
        }
    }

    if (success && speed >= 1 && speed <= 12)
    {
        stationDelay = 60000 / speed / numEnabledStations;
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

    // Retry mechanism for Nextion initialization
    for (uint8_t i = 0; i < 3; i++)
    {
        if (nexInit())
        {
            break;
        }
        delay(50); // Small delay before retrying
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        bReset[i].attachPop(resetStation, &bReset[i]);
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        if (activePage.show())
        {
            break;
        }
        delay(50); // Small delay before retrying
    }
}

void resetStation(NexButton *button)
{
    uint8_t stationToReset = button->getObjPid() -2; // Station 0: Page 2, Station 1: Page 3, etc...
    resetAmps();
    updateEnabledStations();
    failureCounts[stationToReset] = 0;
}

void loop()
{
    nexLoop(nex_listen_list);
    updateMasterEnable();
    if (masterEnable) handleStates();
    else goToSafeState();
}

void goToSafeState()
{
    for (int i = 0; i < 4; i++)
    {
        sendServoMoveCommand(i + 1, 0, 100);
    }  
}