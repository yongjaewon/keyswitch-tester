import sys
import time
import os
from PySide6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QWidget, 
                              QMessageBox, QHBoxLayout, QSplitter, QFrame)
from PySide6.QtCore import QTimer, Qt

from constants import (WINDOW_TITLE, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT,
                      SERIAL_CHECK_INTERVAL, MAIN_WINDOW_STYLE)
from serial_communication import SerialManager
from gui_components import ControlWidget, SerialLogWidget, StationStatusWidget

class KeyswitchTesterGUI(QMainWindow):
    # Define message types for cleaner code
    COMMAND_START = b"START\n"
    COMMAND_STOP = b"STOP\n"
    
    def __init__(self):
        super().__init__()
        self.setup_ui()
        self.setup_connections()
        self.init_serial()
    
    def setup_ui(self):
        """Set up the user interface"""
        # Window properties
        self.setWindowTitle(WINDOW_TITLE)
        self.setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT)
        self.setStyleSheet(MAIN_WINDOW_STYLE)
        
        # Main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        
        main_layout = QHBoxLayout(main_widget)
        main_layout.setContentsMargins(6, 6, 6, 6)
        main_layout.setSpacing(6)
        
        # Left side - Station Status
        self.station_status = StationStatusWidget()
        main_layout.addWidget(self.station_status)
        
        # Separator line
        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)
        separator.setFrameShadow(QFrame.Sunken)
        separator.setStyleSheet("color: #e0e0e0;")
        main_layout.addWidget(separator)
        
        # Right side - Control and Log widgets
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(4)
        
        # Control widget
        self.control_widget = ControlWidget(self.toggle_start_stop)
        right_layout.addWidget(self.control_widget)
        
        # Serial log widget
        self.serial_log = SerialLogWidget()
        right_layout.addWidget(self.serial_log)
        
        main_layout.addWidget(right_widget)
        
        # Set stretch factors
        main_layout.setStretchFactor(self.station_status, 0)
        main_layout.setStretchFactor(separator, 0)
        main_layout.setStretchFactor(right_widget, 1)
    
    def setup_connections(self):
        """Set up signal/slot connections"""
        # Connect station status change signal
        self.station_status.station_state_changed.connect(self.handle_station_state_change)
        
        # Connect reset requested signal
        self.station_status.reset_requested.connect(self.handle_reset_request)
        
        # Initialize timer for serial reading
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(SERIAL_CHECK_INTERVAL)
    
    def init_serial(self):
        """Initialize serial communications"""
        # Initialize serial manager
        self.serial_manager = SerialManager()
        
        # Initial OpenRB check
        port = self.serial_manager.get_openrb_port()
        if port:
            self.log_message(f"OpenRB device found on port: {port}")
        else:
            self.log_message("No OpenRB device found. Connect device and try again.")
    
    def log_message(self, message):
        """Centralized logging method"""
        self.serial_log.append_message(message)
    
    def send_command(self, command, retries=3, retry_delay=0.1):
        """Send command to serial port with retries"""
        if not self.serial_manager.serial_port:
            return False
            
        success = False
        for _ in range(retries):
            if self.serial_manager.write(command):
                success = True
                break  # Exit the loop once successful
            time.sleep(retry_delay)
            
        return success
    
    def handle_station_state_change(self, station_id, enabled):
        """Handle station enable/disable state changes"""
        if not self.serial_manager.serial_port:
            self.log_message("ERROR: Not connected to OpenRB device")
            self.station_status.set_station_enabled(station_id, not enabled)
            return
        
        # Convert station_id to 0-based index for Arduino
        station_idx = station_id - 1
        
        # Send enable/disable command
        command = f"{'ENABLE' if enabled else 'DISABLE'}:{station_idx}\n".encode()
        
        if self.send_command(command):
            self.log_message(f"Station {station_id} {'enabled' if enabled else 'disabled'}")
        else:
            self.log_message(f"ERROR: Failed to send station state change command")
            self.station_status.set_station_enabled(station_id, not enabled)
    
    def handle_reset_request(self, station_id, field_type):
        """Handle requests to reset cycle count or failure count"""
        if not self.serial_manager.serial_port:
            self.log_message("ERROR: Not connected to OpenRB device")
            return
        
        # Convert station_id to 0-based index for Arduino
        station_idx = station_id - 1
        
        # Map field type to command
        command_type = "RESET_CYCLE" if field_type == "cycle_count" else "RESET_FAIL"
        
        # Create and send command
        command = f"{command_type}:{station_idx}\n".encode()
        
        if self.send_command(command):
            self.log_message(f"Reset {field_type.replace('_', ' ')} for Station {station_id}")
        else:
            self.log_message(f"ERROR: Failed to send reset command for {field_type.replace('_', ' ')}")
    
    def connect_to_device(self):
        """Attempt to connect to the OpenRB device"""
        self.log_message("Looking for OpenRB device...")
        port = self.serial_manager.get_openrb_port()
        
        if not port:
            self.log_message("ERROR: No OpenRB device found")
            return False
        
        self.log_message(f"Connecting to OpenRB on {port}...")
        
        if not self.serial_manager.connect(port):
            self.log_message(f"ERROR: Failed to connect to {port}")
            return False
        
        self.log_message(f"Connected to OpenRB on {port}")
        
        # Request current state from Arduino after successful connection
        self.request_current_state()
        
        return True
        
    def request_current_state(self):
        """Request current state from Arduino"""
        self.log_message("Requesting current state from Arduino...")
        command = b"REQUEST_STATE\n"
        
        if self.send_command(command):
            self.log_message("State request sent to Arduino")
        else:
            self.log_message("ERROR: Failed to send state request")
            
    def toggle_start_stop(self):
        """Toggle between connect, start, and stop states"""
        # If not connected (button shows "Connect"), try to connect
        if not self.serial_manager.serial_port:
            if self.connect_to_device():
                # Connection successful, update button to "Start"
                self.control_widget.update_state(False)  # Set to "Start" (not running)
            else:
                # Connection failed, keep as "Connect"
                self.log_message("Failed to connect to device")
            return
        
        # Already connected, handle Start/Stop toggling
        is_running = self.control_widget.start_stop_btn.isChecked()
        command = self.COMMAND_START if is_running else self.COMMAND_STOP
        status = "Started" if is_running else "Stopped"
        
        if self.send_command(command):
            self.log_message(status)
        else:
            self.log_message("ERROR: Failed to send command")
            if is_running:  # Only revert UI if we're trying to start
                self.control_widget.update_state(False)
                return
        
        # No need to call update_state here as it was already toggled by the button's checked state
    
    def read_serial(self):
        """Read and process data from the serial port"""
        line = self.serial_manager.read_line()
        if line:
            self.log_message(line)
            self.process_serial_message(line)
    
    def process_serial_message(self, message):
        """Process serial messages and update the station status if applicable"""
        try:
            message = message.strip()
            
            # Handle CYCLE format: "CYCLE:station:enabled:cycles:fails:keyswitchCurrent:starterCurrent"
            if message.startswith("CYCLE:"):
                self._process_cycle_message(message)
            # Handle system state updates
            elif message.startswith("SYSTEM_STATE:"):
                self._process_system_state_message(message)
            # Handle station state updates
            elif message.startswith("STATION:"):
                self._process_station_message(message)
        except Exception as e:
            self.log_message(f"ERROR: Failed to process message: {str(e)}")
    
    def _process_cycle_message(self, message):
        """Process messages in the CYCLE:station:enabled:cycles:fails:keyswitchCurrent:starterCurrent format"""
        parts = message.split(":")
        
        if len(parts) >= 7:  # Updated to expect 7 parts
            # Extract values (station 0 = display station 1)
            station_idx = int(parts[1]) + 1  # Convert to 1-based for display
            station_enabled = parts[2] == "1"  # "1" = enabled, "0" = disabled
            cycle_count = int(parts[3])
            failure_count = int(parts[4])
            keyswitch_current = float(parts[5])
            starter_current = float(parts[6])
            
            # Validate station number (1-4 for display)
            if 1 <= station_idx <= 4:
                # Update all values at once
                self.station_status.set_station_enabled(station_idx, station_enabled)
                self.station_status.update_station_value(station_idx, 'cycle_count', cycle_count)
                self.station_status.update_station_value(station_idx, 'failure_count', failure_count)
                self.station_status.update_station_value(station_idx, 'keyswitch_current', keyswitch_current)
                self.station_status.update_station_value(station_idx, 'starter_current', starter_current)
            else:
                self.log_message(f"ERROR: Invalid station number: {station_idx-1} (must be 0-3)")
        else:
            self.log_message(f"ERROR: Invalid CYCLE message format: {message}, expected 7 parts but got {len(parts)}")
    
    def _process_system_state_message(self, message):
        """Process system state message from Arduino"""
        parts = message.split(":")
        
        if len(parts) < 2:
            self.log_message(f"ERROR: Invalid system state message format: {message}")
            return
            
        try:
            is_running = parts[1] == "1"  # "1" = running, "0" = stopped
            self.control_widget.update_state(is_running)
            self.log_message(f"System state updated: {'running' if is_running else 'stopped'}")
        except ValueError as e:
            self.log_message(f"ERROR: Invalid system state value: {e}")
    
    def _process_station_message(self, message):
        """Process station state message from Arduino"""
        parts = message.split(":")
        
        if len(parts) >= 5:
            try:
                station_idx = int(parts[1]) + 1  # Convert to 1-based for display
                station_enabled = parts[2] == "1"  # "1" = enabled, "0" = disabled
                cycle_count = int(parts[3])
                failure_count = int(parts[4])
                
                # Validate station number (1-4 for display)
                if 1 <= station_idx <= 4:
                    # Update all values except currents which aren't included in station message
                    self.station_status.set_station_enabled(station_idx, station_enabled)
                    self.station_status.update_station_value(station_idx, 'cycle_count', cycle_count)
                    self.station_status.update_station_value(station_idx, 'failure_count', failure_count)
                else:
                    self.log_message(f"ERROR: Invalid station number: {station_idx-1} (must be 0-3)")
            except ValueError as e:
                self.log_message(f"ERROR: Invalid station state value: {e}")
        else:
            self.log_message(f"ERROR: Invalid STATION message format: {message}, expected 5 parts but got {len(parts)}")

def main():
    """Application entry point"""
    app = QApplication(sys.argv)
    window = KeyswitchTesterGUI()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main() 