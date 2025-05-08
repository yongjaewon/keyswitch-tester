# Serial communication settings
SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 0.1
SERIAL_CHECK_INTERVAL = 100  # ms

# GUI settings
WINDOW_TITLE = "Keyswitch Tester"
WINDOW_MIN_WIDTH = 800
WINDOW_MIN_HEIGHT = 500

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