# Serial communication settings
SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 0.1
SERIAL_CHECK_INTERVAL = 100  # ms

# GUI settings
WINDOW_TITLE = "Keyswitch Tester"
WINDOW_MIN_WIDTH = 400
WINDOW_MIN_HEIGHT = 300

# Number of stations
NUM_STATIONS = 4

# Current threshold for determining failures (in mA)
CURRENT_THRESHOLD = 5.0  # Adjust this value based on your requirements

# Button styles
START_BUTTON_STYLE = """
    QPushButton {
        background-color: #4CAF50;
        color: white;
        border: none;
        padding: 10px;
        font-size: 14px;
        border-radius: 4px;
    }
    QPushButton:checked {
        background-color: #f44336;
    }
"""

RESET_BUTTON_STYLE = """
    QPushButton {
        background-color: #2196F3;
        color: white;
        border: none;
        padding: 5px;
        border-radius: 4px;
    }
""" 