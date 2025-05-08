import sys
import time
from PySide6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QWidget, 
                              QMessageBox, QHBoxLayout, QSplitter, QFrame)
from PySide6.QtCore import QTimer, Qt

from constants import (WINDOW_TITLE, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT,
                      SERIAL_CHECK_INTERVAL)
from serial_communication import SerialManager
from gui_components import ControlWidget, SerialLogWidget, StationStatusWidget

class KeyswitchTesterGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle(WINDOW_TITLE)
        self.setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT)
        
        # Set window style
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f9f9f9;
            }
        """)
        
        # Initialize serial manager
        self.serial_manager = SerialManager()
        
        # Create main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        
        # Main layout will be horizontal (left and right sections)
        main_layout = QHBoxLayout(main_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # Left side - Station Status
        self.station_status = StationStatusWidget()
        self.station_status.station_state_changed.connect(self.handle_station_state_change)
        main_layout.addWidget(self.station_status)
        
        # Separator line between sections
        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)
        separator.setFrameShadow(QFrame.Sunken)
        separator.setStyleSheet("color: #e0e0e0;")
        main_layout.addWidget(separator)
        
        # Right side - Control and Log widgets
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(10)
        
        # Add control widget
        self.control_widget = ControlWidget(self.toggle_start_stop)
        right_layout.addWidget(self.control_widget)
        
        # Add serial log widget
        self.serial_log = SerialLogWidget()
        right_layout.addWidget(self.serial_log)
        
        # Add right widget to main layout
        main_layout.addWidget(right_widget)
        
        # Set stretch factors to make right side expand when resizing
        main_layout.setStretchFactor(self.station_status, 0)  # Don't stretch
        main_layout.setStretchFactor(separator, 0)            # Don't stretch
        main_layout.setStretchFactor(right_widget, 1)         # Stretch to fill space
        
        # Initialize timer for serial reading
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(SERIAL_CHECK_INTERVAL)
        
        # Initial OpenRB check
        openrb_port = self.serial_manager.get_openrb_port()
        if openrb_port:
            self.serial_log.append_message(f"OpenRB device found on port: {openrb_port}")
        else:
            self.serial_log.append_message("No OpenRB device found. Connect device and try again.")
    
    def handle_station_state_change(self, station_id, enabled):
        """Handle station enable/disable state changes"""
        if not self.serial_manager.serial_port:
            self.serial_log.append_message("ERROR: Not connected to OpenRB device")
            # Revert the toggle state
            self.station_status.set_station_enabled(station_id, not enabled)
            return
        
        # Convert station_id to 0-based index for Arduino
        station_idx = station_id - 1
        
        # Send enable/disable command
        command = f"{'ENABLE' if enabled else 'DISABLE'}:{station_idx}\n".encode()
        
        # Send command multiple times to ensure delivery
        success = False
        for _ in range(3):
            if self.serial_manager.write(command):
                success = True
            time.sleep(0.1)
        
        if success:
            self.serial_log.append_message(f"Station {station_id} {'enabled' if enabled else 'disabled'}")
        else:
            self.serial_log.append_message(f"ERROR: Failed to send station state change command")
            # Revert the toggle state
            self.station_status.set_station_enabled(station_id, not enabled)
    
    def toggle_start_stop(self):
        """Toggle between start and stop states"""
        # Try to connect to port if not already connected
        if not self.serial_manager.serial_port:
            self.serial_log.append_message("Looking for OpenRB device...")
            openrb_port = self.serial_manager.get_openrb_port()
            
            if not openrb_port:
                self.serial_log.append_message("ERROR: No OpenRB device found")
                self.control_widget.update_state(False)
                return
            
            # Try to connect
            self.serial_log.append_message(f"Connecting to OpenRB on {openrb_port}...")
            
            if not self.serial_manager.connect(openrb_port):
                self.serial_log.append_message(f"ERROR: Failed to connect to {openrb_port}")
                self.control_widget.update_state(False)
                return
            
            self.serial_log.append_message(f"Connected to OpenRB on {openrb_port}")
        
        # Send start/stop command
        is_running = self.control_widget.start_stop_btn.isChecked()
        command = b"START\n" if is_running else b"STOP\n"
        status = "Started" if is_running else "Stopped"
        
        # Send command multiple times to ensure delivery
        success = False
        for _ in range(3):
            if self.serial_manager.write(command):
                success = True
            time.sleep(0.1)
        
        if success:
            self.serial_log.append_message(status)
        else:
            self.serial_log.append_message(f"ERROR: Failed to send command")
            if is_running:  # Only revert UI if we're trying to start
                self.control_widget.update_state(False)
                return
        
        self.control_widget.update_state(is_running)
    
    def read_serial(self):
        """Read and process data from the serial port"""
        line = self.serial_manager.read_line()
        if line:
            # Log the raw message
            self.serial_log.append_message(line)
            
            # Process the message to update station status if applicable
            self.process_serial_message(line)
    
    def process_serial_message(self, message):
        """Process serial messages and update the station status if applicable
        
        Expected message formats:
        - CYCLE:A:B:C:D - Where A=station (0-3), B=cycle count, C=failure count, D=current in Amps
        - STATION:<id>:CYCLE:<count> - Legacy format for cycle count
        - STATION:<id>:FAIL:<count> - Legacy format for failure count
        - STATION:<id>:CURRENT:<amps> - Legacy format for current reading
        """
        try:
            message = message.strip()
            
            # Handle new CYCLE format: "CYCLE:A:B:C:D"
            if message.startswith("CYCLE:"):
                parts = message.split(":")
                
                if len(parts) >= 5:
                    # Extract values (station 0 = display station 1)
                    station_idx = int(parts[1]) + 1  # Convert to 1-based for display
                    cycle_count = int(parts[2])
                    failure_count = int(parts[3])
                    current = float(parts[4])
                    
                    # Validate station number (1-4 for display)
                    if 1 <= station_idx <= 4:
                        # Update all three values at once
                        self.station_status.update_station_value(station_idx, 'cycle_count', cycle_count)
                        self.station_status.update_station_value(station_idx, 'failure_count', failure_count)
                        self.station_status.update_station_value(station_idx, 'current', current)
                    else:
                        self.serial_log.append_message(f"ERROR: Invalid station number: {station_idx-1} (must be 0-3)")
                else:
                    self.serial_log.append_message(f"ERROR: Invalid CYCLE message format: {message}")
                
                return  # Done processing this message
            
            # Legacy format handling
            elif message.startswith("STATION:"):
                parts = message.split(":")
                
                if len(parts) >= 4:
                    station_id = int(parts[1])
                    data_type = parts[2]
                    value = parts[3]
                    
                    # Map data types to widget value types
                    type_mapping = {
                        "CYCLE": "cycle_count",
                        "FAIL": "failure_count",
                        "CURRENT": "current"
                    }
                    
                    if data_type in type_mapping and 1 <= station_id <= 4:
                        # Convert value to appropriate type
                        if data_type in ["CYCLE", "FAIL"]:
                            try:
                                value = int(value)
                            except ValueError:
                                self.serial_log.append_message(f"ERROR: Invalid value format for {data_type}: {value}")
                                return
                        elif data_type == "CURRENT":
                            try:
                                value = float(value)
                            except ValueError:
                                self.serial_log.append_message(f"ERROR: Invalid current value: {value}")
                                return
                        
                        # Update the station status widget
                        self.station_status.update_station_value(
                            station_id, 
                            type_mapping[data_type], 
                            value
                        )
        except Exception as e:
            # Log any errors during processing
            self.serial_log.append_message(f"ERROR: Failed to process message: {str(e)}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = KeyswitchTesterGUI()
    window.show()
    sys.exit(app.exec()) 