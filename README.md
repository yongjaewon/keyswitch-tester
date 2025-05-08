# Keyswitch Tester

A system for automated testing of keyswitches using a Raspberry Pi and Arduino-based control system.

## Project Structure

```
keyswitch-tester/
├── arduino/              # Arduino control code
│   ├── commandParser.cpp # Command handling
│   ├── config.h         # Configuration settings
│   ├── currentMeasurer.cpp # Current measurement
│   ├── eventReporter.cpp   # Event reporting
│   └── stateMachine.cpp    # State management
├── gui/                  # Python GUI application
│   ├── main.py          # Main application window
│   ├── gui_components.py # GUI components
│   ├── constants.py     # GUI constants
│   └── serial_communication.py # Serial communication
├── logs/                # Log files directory
└── requirements.txt     # Python dependencies
```

## Features

- Automated testing of up to 4 keyswitch stations
- Real-time current measurement and monitoring
- Cycle counting and failure detection
- Individual station enable/disable control
- Emergency stop functionality
- Detailed logging system
- Modern GUI interface

## Requirements

### Hardware
- Raspberry Pi 5
- Robotis OpenRB-150 board (Arduino-compatible)
- Robotis Dynamixel XM430-W210-T

### Software
- Python 3.8+
- Arduino IDE
- Required Python packages (see requirements.txt)

## Setup

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd keyswitch-tester
   ```

2. Set up Python environment:
   ```bash
   python -m venv .venv
   source .venv/bin/activate  # On Windows: .venv\Scripts\activate
   pip install -r requirements.txt
   ```

3. Upload Arduino code:
   ```bash
   cd arduino
   ./upload.sh
   ```

4. Run the GUI:
   ```bash
   python gui/main.py
   ```

## Usage

1. Connect the OpenRB board to any USB port on the Raspberry Pi (the system will automatically detect it)
2. Launch the GUI application
3. Use the Start/Stop button to control the testing system
4. Monitor station status in real-time
5. Enable/disable individual stations using the toggle sliders
6. View detailed logs in the log window
7. Save logs for later analysis

## Logging

- Logs are automatically saved in the `logs` directory
- Each session creates a new log file with timestamp
- Logs can be manually saved using the Save Log button
- Log format includes timestamps and detailed event information

## Safety Features

- Emergency stop functionality
- Current monitoring and failure detection
- Station isolation capability
- Automatic error logging