import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget, QMessageBox
from PySide6.QtCore import QTimer

from constants import (WINDOW_TITLE, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT,
                      NUM_STATIONS, SERIAL_CHECK_INTERVAL, CURRENT_THRESHOLD)
from serial_communication import SerialManager
from gui_components import (StationWidget, SerialPortWidget, ControlWidget,
                          SerialLogWidget)

class KeyswitchTesterGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle(WINDOW_TITLE)
        self.setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT)
        
        # Initialize serial manager
        self.serial_manager = SerialManager()
        self.cycle_counts = [0] * NUM_STATIONS
        self.failure_counts = [0] * NUM_STATIONS
        
        # Create main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Create and add widgets
        self.serial_port_widget = SerialPortWidget(self.refresh_ports)
        layout.addWidget(self.serial_port_widget)
        
        self.control_widget = ControlWidget(self.toggle_start_stop)
        layout.addWidget(self.control_widget)
        
        # Create station widgets
        self.station_widgets = []
        for i in range(NUM_STATIONS):
            station_widget = StationWidget(i, self.reset_station)
            self.station_widgets.append(station_widget)
            layout.addWidget(station_widget)
        
        # Create and add serial log widget
        self.serial_log = SerialLogWidget()
        layout.addWidget(self.serial_log)
        
        # Initialize timer for serial reading
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(SERIAL_CHECK_INTERVAL)
        
        # Initial port refresh
        self.refresh_ports()
    
    def refresh_ports(self):
        """Refresh the list of available serial ports"""
        ports = self.serial_manager.get_available_ports()
        self.serial_port_widget.update_ports(ports)
    
    def toggle_start_stop(self):
        """Toggle between start and stop states"""
        if not self.serial_manager.serial_port:
            port = self.serial_port_widget.get_selected_port()
            if not self.serial_manager.connect(port):
                self.control_widget.update_state(False)
                return
        
        is_running = self.control_widget.start_stop_btn.isChecked()
        if is_running:
            self.serial_manager.write(b"START\n")
        else:
            self.serial_manager.write(b"STOP\n")
        
        self.control_widget.update_state(is_running)
    
    def reset_station(self, station_idx):
        """Reset both cycle and failure counts for a specific station"""
        if 0 <= station_idx < NUM_STATIONS:
            self.cycle_counts[station_idx] = 0
            self.failure_counts[station_idx] = 0
            self.station_widgets[station_idx].update_counts(0, 0)
    
    def show_event_message(self, message):
        """Display an event message to the user"""
        QMessageBox.information(self, "Event", message)
    
    def read_serial(self):
        """Read and process data from the serial port"""
        line = self.serial_manager.read_line()
        if not line:
            return
            
        # Log the raw message
        self.serial_log.append_message(line)
            
        # Handle current report messages
        current_report = self.serial_manager.parse_current_report(line)
        if current_report is not None:
            station_idx, current = current_report
            if 0 <= station_idx < NUM_STATIONS:
                # Always increment cycle count
                self.cycle_counts[station_idx] += 1
                
                # Only increment failure count if current is below threshold
                if current < CURRENT_THRESHOLD:
                    self.failure_counts[station_idx] += 1
                
                # Update the display
                self.station_widgets[station_idx].update_counts(
                    self.cycle_counts[station_idx],
                    self.failure_counts[station_idx]
                )
            return
            
        # Handle event messages
        event_message = self.serial_manager.parse_event_message(line)
        if event_message is not None:
            self.show_event_message(event_message)
            return
            
        # Handle failure messages (legacy format)
        if line.startswith("FAIL:"):
            try:
                station = int(line[5:])
                if 0 <= station < NUM_STATIONS:
                    self.failure_counts[station] += 1
                    self.station_widgets[station].update_counts(
                        self.cycle_counts[station],
                        self.failure_counts[station]
                    )
            except ValueError:
                pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = KeyswitchTesterGUI()
    window.show()
    sys.exit(app.exec()) 