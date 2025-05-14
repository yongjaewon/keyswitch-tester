# Serial communication settings
SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 0.1
SERIAL_CHECK_INTERVAL = 100  # ms

# GUI settings
WINDOW_TITLE = "Keyswitch Tester"
WINDOW_MIN_WIDTH = 800
WINDOW_MIN_HEIGHT = 250

# Colors
COLOR_PRIMARY = "#2196F3"    # Blue
COLOR_SUCCESS = "#4CAF50"    # Green
COLOR_ERROR = "#f44336"      # Red
COLOR_DISABLED = "#e0e0e0"   # Light Gray
COLOR_BACKGROUND = "#f9f9f9" # Off-white
COLOR_TEXT_DARK = "#333333"  # Dark Gray
COLOR_TEXT_LIGHT = "white"   # White

# Styles
MAIN_WINDOW_STYLE = f"""
    QMainWindow {{
        background-color: {COLOR_BACKGROUND};
    }}
"""

TOGGLE_BUTTON_STYLE = f"""
    QPushButton {{
        border: none;
        border-radius: 4px;
        font-weight: bold;
        font-size: 12px;
    }}
    QPushButton:checked {{
        background-color: {COLOR_SUCCESS};
        color: {COLOR_TEXT_LIGHT};
    }}
    QPushButton:!checked {{
        background-color: {COLOR_DISABLED};
        color: {COLOR_TEXT_DARK};
    }}
"""

START_BUTTON_STYLE = f"""
    QPushButton {{
        background-color: {COLOR_SUCCESS};
        color: {COLOR_TEXT_LIGHT};
        border: none;
        padding: 6px;
        font-size: 14px;
        border-radius: 4px;
        min-height: 32px;
    }}
    QPushButton:checked {{
        background-color: {COLOR_ERROR};
    }}
    QPushButton:hover {{
        background-color: #45a049;
    }}
    QPushButton:checked:hover {{
        background-color: #e53935;
    }}
"""

RESET_BUTTON_STYLE = f"""
    QPushButton {{
        background-color: {COLOR_PRIMARY};
        color: {COLOR_TEXT_LIGHT};
        border: none;
        padding: 5px;
        border-radius: 4px;
    }}
"""

LOG_BUTTON_STYLE = f"""
    QPushButton {{ 
        background-color: {COLOR_BACKGROUND}; 
        border: 1px solid #ddd; 
        padding: 4px 8px; 
        border-radius: 3px; 
    }}
"""

# Station styling
STATION_FRAME_STYLE = """
    background-color: white; 
    border: 1px solid #e0e0e0; 
    border-radius: 4px;
"""

STATION_LABEL_STYLE = f"""
    color: {COLOR_PRIMARY}; 
    font-weight: bold;
""" 