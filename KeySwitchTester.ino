#include <Nextion.h>
#include "Hiwonder.h"

Nextion *nex = Nextion::GetInstance(Serial2);

// Define Nextion components

NexPage activePage = NexPage(nex, 1, "active");
NexPage popup0 = NexPage(nex, 2, "popup0");
NexPage popup1 = NexPage(nex, 3, "popup1");
NexPage popup2 = NexPage(nex, 4, "popup2");
NexPage popup3 = NexPage(nex, 5, "popup3");

NexNumber nTarget[] =
    {
        NexNumber(nex, 1, 1, "nT0", &activePage),
        NexNumber(nex, 1, 3, "nT1", &activePage),
        NexNumber(nex, 1, 5, "nT2", &activePage),
        NexNumber(nex, 1, 7, "nT3", &activePage)};

NexNumber nCounter[] =
    {
        NexNumber(nex, 1, 2, "nC0", &activePage),
        NexNumber(nex, 1, 4, "nC1", &activePage),
        NexNumber(nex, 1, 6, "nC2", &activePage),
        NexNumber(nex, 1, 8, "nC3", &activePage)};

NexCrop motorState[] =
    {
        NexCrop(nex, 1, 10, "crM0", &activePage),
        NexCrop(nex, 1, 12, "crM1", &activePage),
        NexCrop(nex, 1, 14, "crM2", &activePage),
        NexCrop(nex, 1, 16, "crM3", &activePage)};

NexCrop keyState[] =
    {
        NexCrop(nex, 1, 11, "crK0", &activePage),
        NexCrop(nex, 1, 13, "crK1", &activePage),
        NexCrop(nex, 1, 15, "crK2", &activePage),
        NexCrop(nex, 1, 17, "crK3", &activePage)};

NexNumber nMotorFails[] =
    {
        NexNumber(nex, 1, 20, "nMF0", &activePage),
        NexNumber(nex, 1, 21, "nMF1", &activePage),
        NexNumber(nex, 1, 22, "nMF2", &activePage),
        NexNumber(nex, 1, 23, "nMF3", &activePage)};

NexNumber nKeyFails[] =
    {
        NexNumber(nex, 1, 24, "nKF0", &activePage),
        NexNumber(nex, 1, 25, "nKF1", &activePage),
        NexNumber(nex, 1, 26, "nKF2", &activePage),
        NexNumber(nex, 1, 27, "nKF3", &activePage)};

NexDSButton stationEnable[] =
    {
        NexDSButton(nex, 1, 28, "dsS0", &activePage),
        NexDSButton(nex, 1, 29, "dsS1", &activePage),
        NexDSButton(nex, 1, 30, "dsS2", &activePage),
        NexDSButton(nex, 1, 31, "dsS3", &activePage)};

NexNumber keyAmps[] =
    {
        NexNumber(nex, 1, 35, "fAmps0", &activePage),
        NexNumber(nex, 1, 36, "fAmps1", &activePage),
        NexNumber(nex, 1, 37, "fAmps2", &activePage),
        NexNumber(nex, 1, 38, "fAmps3", &activePage)};

NexButton bReset[] =
    {
        NexButton(nex, 2, 3, "b1", &popup0),
        NexButton(nex, 3, 3, "b1", &popup1),
        NexButton(nex, 4, 3, "b1", &popup2),
        NexButton(nex, 5, 3, "b1", &popup3)};

NexDSButton dsEnable = NexDSButton(nex, 1, 9, "dsEnable", &activePage);
NexCrop sdState = NexCrop(nex, 1, 18, "crSD", &activePage);
NexText msg = NexText(nex, 1, 32, "tMsg", &activePage);
NexCrop battState = NexCrop(nex, 1, 33, "crVBatt", &activePage);
NexNumber vBatt = NexNumber(nex, 1, 34, "fVBatt", &activePage);
NexNumber cpm = NexNumber(nex, 1, 39, "nCPM", &activePage);

// List of components to listen for events
NexTouch *nex_listen_list[] =
    {
        &bReset[0],
        &bReset[1],
        &bReset[2],
        &bReset[3],
        NULL};

// Servo and current sensor parameters
const uint16_t T_DWELL = 600;
const uint16_t SERVO_WAIT_TIME = 300;
const uint16_t ZERO_POINT[4] = {565, 525, 528, 595}; // Zero points for each station
const float V_REF = 5.0;                             // Arduino reference voltage
const float ADC_RESOLUTION = 1023.0;                 // 10-bit ADC resolution
const float V_OFFSET = 2.5;                          // Zero-current voltage (2.5V)
const float SENSITIVITY = 0.04;                      // Sensitivity in V/A (40 mV/A for ACS758-050B)
const uint8_t AMPS_PIN = A0;                         // Current sensor connected to A0
const uint8_t AMPS_THRESHOLD = 5;                    // Failure threshold: 5A
const uint8_t FAILURE_THRESHOLD = 10;                // Total number of failures threshold: 10
const uint8_t AMPS_N_TOP = 10;                        // Number of top readings to maintain
const uint8_t STATE_DELAY = 50;
const uint8_t V_BATT_PIN = A2;

uint16_t stationDelay = 0;    // Time delay between stations
unsigned long tlastCycle = 0; // Timer to track periodic updates from Nextion
bool masterEnable = false;    // Master enable state
unsigned long tLastMasterEnUpdate = 0;

float topReadings[AMPS_N_TOP]; // Array to store top N readings
uint8_t topCount = 0;          // Current number of readings in topReadings

uint32_t measurementCounter = 0;

// Display state colors
enum Color
{
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
    SERVO_RETURN,
    SERVO_WAIT
};

State currentState = IDLE;                 // Initial state
ServoSubState servoSubState = SERVO_START; // Initial substate for servo commands

unsigned long lastActuationTime = 0; // Timer to track periodic actuation
unsigned long lastStateTime = 0;     // Timer for state transitions

bool enabledStations[4] = {0, 0, 0, 0}; // Store which stations are enabled
uint8_t numEnabledStations = 0;         // Count of enabled stations
uint8_t currentStationIndex = -1;       // Current station being processed

// uint32_t failureCounts[4] = {8, 4, 1, 0};

void insertTopReading(float current)
{
    // If we haven't filled the topReadings array yet
    if (topCount < AMPS_N_TOP)
    {
        topReadings[topCount] = current;
        topCount++;

        // Sort the array in descending order
        for (int8_t i = topCount - 1; i > 0; i--)
        {
            if (topReadings[i] > topReadings[i - 1])
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
        if (current > topReadings[AMPS_N_TOP - 1])
        {
            topReadings[AMPS_N_TOP - 1] = current;

            // Move the new reading up to maintain descending order
            for (int i = AMPS_N_TOP - 1; i > 0; i--)
            {
                if (topReadings[i] > topReadings[i - 1])
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
void sendData(byte data[], int length)
{
    for (int i = 0; i < length; i++)
    {
        // Serial.print(data[i], HEX);
        // Serial.print(", ");
        Serial1.write(data[i]); // Send data to the servo controller
    }
    // Serial.println();
}

// Function to create and send a command for servo movement
void sendServoMoveCommand(byte servoID, int angle, int time)
{
    byte data[10];

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
        masterEnable = (bool)isEnabled;
}

// Function to read current and update the top readings
void monitorCurrent()
{
    uint16_t adcValue = analogRead(AMPS_PIN);
    float vInput = (adcValue / ADC_RESOLUTION) * V_REF;
    float current = (V_OFFSET - vInput) / SENSITIVITY;

    insertTopReading(max(0, current));
}

uint32_t calculateAvgAmps()
{
    // If there are no readings, return 0
    if (topCount == 0)
    {
        return 0;
    }

    // Calculate the sum of the top readings
    float sum = 0.0f;
    for (uint8_t i = 0; i < topCount; i++)
    {
        sum += topReadings[i];
    }
    float average = sum / topCount;

    if (average < 0)
        return 0;
    // Multiply by 10 to shift decimal, add 0.5 for rounding, and cast to int
    else
        return (uint32_t)(average * 10.0f + 0.5f); // Returns the average value as an integer in tenth of an Amp
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

        if (success)
        {
            enabledStations[i] = isEnabled;
            if (isEnabled)
                numEnabledStations++;
        }
    }
}

void calculateAmps()
{
    uint32_t avgCurrent = calculateAvgAmps();
    // Serial.println(avgCurrent);
    for (uint8_t i = 0; i < 3; i++)
    {
        if (keyAmps[currentStationIndex].setValue(avgCurrent))
            break;
    }

    // Serial.print("Current: ");
    // Serial.println(avgCurrent);


    uint32_t fails;
    nKeyFails[currentStationIndex].getValue(&fails);
    if (avgCurrent < 50) // If less than 5.0 Amps
    {
        fails++;
        nKeyFails[currentStationIndex].setValue(fails);
        // failureCounts[currentStationIndex]++;
        // for (uint8_t i = 0; i < 4; i++)
        // {
        //     if (nKeyFails[currentStationIndex].setValue(failureCounts[currentStationIndex]))
        //         break;
        // }
    }

    // Serial.print(failureCounts[currentStationIndex]);
    // Serial.print(" >= ");
    // Serial.print(FAILURE_THRESHOLD);
    // Serial.print("?: ");
    // Serial.println(failureCounts[currentStationIndex] >= FAILURE_THRESHOLD? "true" : "false");

    if (fails >= FAILURE_THRESHOLD)
    {
        // for (uint8_t i = 0; i < 3; i++)
        // {
        //     if (stationEnable[currentStationIndex].setValue(0))
        //         break;
        // }
        stationEnable[currentStationIndex].setValue(0);
    }

    resetAmps();
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
    static uint32_t currentCount = 0; // Track the current count locally

    switch (currentState)
    {
    case IDLE:
        if (millis() - lastActuationTime >= stationDelay)
        {
            updateEnabledStations();
            updateStationDelay();
            if (numEnabledStations > 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    currentStationIndex = (currentStationIndex + 1) % 4;
                    if (enabledStations[currentStationIndex])
                        break;
                }
                currentState = GET_VALUE;
                lastActuationTime = millis();
            }
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
            }
            measurementCounter = 0;
            lastStateTime = millis();
        }
        break;

    case SERVO_COMMAND:
        monitorCurrent(); // Continuously monitor current during this state
        measurementCounter++;
        switch (servoSubState)
        {
        case SERVO_START:
            sendServoMoveCommand(currentStationIndex + 1, 105, 200); // Actuate the current station's key switch
            servoSubState = SERVO_RETURN;
            lastStateTime = millis();
            break;

        case SERVO_RETURN:
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
        Serial.println(measurementCounter);
        for (int i = 0; i < AMPS_N_TOP; i++)
        {
            Serial.print(topReadings[i]);
            Serial.print(", ");
        }
        Serial.println();
        calculateAmps();
        resetAmps();
        updateVBatt();
        currentState = IDLE; // Reset for the next station
        break;
    }
}

void updateStationDelay()
{
    bool success = false;
    uint32_t speed;

    for (uint8_t attempt = 0; attempt < 3; attempt++)
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
    pinMode(V_BATT_PIN, INPUT);
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize Serial1 for Hiwonder Servo Controller
    Serial1.begin(9600);

    // // nexSerial for Nextion defaults to Serial2
    // nexSerial.begin(115200);
    // if(!nex->nexInit(115200))
    // {
    //     Serial.println("nextion init fails"); 
    // }
    // delay(500);

    // Retry mechanism for Nextion initialization
    for (uint8_t i = 0; i < 3; i++)
    {
        if (!nex->nexInit(115200))
        {
            break;
        }
        delay(50); // Small delay before retrying
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            bReset[i].attachPop(resetStation, &bReset[i]);
        }
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        if (activePage.show())
        {
            break;
        }
        delay(50); // Small delay before retrying
    }

    // for (uint8_t i = 0; i < 4; i++)
    // {
    //     for (uint8_t j = 0; j < 3; j++)
    //     {
    //         if (nKeyFails[i].setValue(failureCounts[i]))
    //             break;
    //     }
    // }
}

void resetStation(void* ptr)
{
    Serial.print("Resetting station ");
    NexButton* button = static_cast<NexButton*>(ptr);
    uint8_t stationToReset = button->getObjPid() - 2; // Station 0: Page 2, Station 1: Page 3, etc...
    Serial.println(stationToReset + 1);
    nKeyFails[stationToReset].setValue(0);
    // failureCounts[stationToReset] = 0;
    // for (uint8_t i = 0; i < 4; i++)
    // {
    //     Serial.print(failureCounts[i]);
    //     Serial.print(", ");
    // }
    // Serial.println();
    updateEnabledStations();
}

void updateVBatt()
{
    float vBattFraction = analogRead(V_BATT_PIN) / ADC_RESOLUTION;
    uint32_t vBattInt = (uint32_t)(vBattFraction * 650 / 3 + 0.5);
    vBatt.setValue(vBattInt);
}

void loop()
{
    nex->nexLoop(nex_listen_list);
    if (millis() - tLastMasterEnUpdate >= 100)
    {
      tLastMasterEnUpdate = millis();
      updateMasterEnable();
    }
    if (masterEnable)
        handleStates();
    else
        goToSafeState();
}

void goToSafeState()
{
    for (int i = 0; i < 4; i++)
    {
        sendServoMoveCommand(i + 1, 0, 100);
    }
    delay(100);
}	 