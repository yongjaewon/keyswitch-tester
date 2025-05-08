from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, 
                             QPushButton, QLabel, QTextEdit, QFileDialog,
                             QGridLayout, QFrame, QSlider)
from PySide6.QtCore import QDateTime, Qt, Signal
from PySide6.QtGui import QFont
import os
from constants import START_BUTTON_STYLE, RESET_BUTTON_STYLE

class SerialLogWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)
        
        # Header with controls
        header_layout = QHBoxLayout()
        
        # Title
        log_title = QLabel("Log")
        log_title.setFont(QFont("Arial", 9, QFont.Bold))
        header_layout.addWidget(log_title)
        
        header_layout.addStretch()
        
        # Controls with consistent styling
        button_style = "QPushButton { background-color: #f5f5f5; border: 1px solid #ddd; padding: 4px 8px; border-radius: 3px; }"
        
        # Add save button
        save_btn = QPushButton("Save Log")
        save_btn.setStyleSheet(button_style)
        save_btn.clicked.connect(self.save_log)
        header_layout.addWidget(save_btn)
        
        # Add clear button
        clear_btn = QPushButton("Clear")
        clear_btn.setStyleSheet(button_style)
        clear_btn.clicked.connect(self.clear)
        header_layout.addWidget(clear_btn)
        
        layout.addLayout(header_layout)
        
        # Text display with better styling
        self.text_display = QTextEdit()
        self.text_display.setReadOnly(True)
        self.text_display.setStyleSheet("background-color: white; border: 1px solid #e0e0e0; border-radius: 3px;")
        self.text_display.setMaximumHeight(150)
        layout.addWidget(self.text_display)
        
        # Initialize logging variables
        self.log_file = None
        self.log_file_path = None
        
        # Start auto-logging immediately
        self.start_auto_logging()
        
        self.setLayout(layout)
    
    def append_message(self, message):
        """Append a message to the log"""
        # Add timestamp to message
        timestamp = QDateTime.currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
        formatted_message = f"[{timestamp}] {message}"
        
        # Add to display
        self.text_display.append(formatted_message)
        
        # Save to file if log file is open
        if self.log_file:
            try:
                self.log_file.write(formatted_message + "\n")
                self.log_file.flush()  # Make sure it's written immediately
            except Exception as e:
                # If there's an error writing, disable auto-save
                self.text_display.append(f"[ERROR] Failed to write to log file: {str(e)}")
                self.restart_auto_logging()
    
    def start_auto_logging(self):
        """Automatically start logging to a file in the logs directory"""
        # Create logs directory if it doesn't exist
        logs_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "logs")
        os.makedirs(logs_dir, exist_ok=True)
        
        # Default filename with timestamp
        default_filename = f"keyswitch_log_{QDateTime.currentDateTime().toString('yyyyMMdd_HHmmss')}.txt"
        file_path = os.path.join(logs_dir, default_filename)
        
        try:
            # Close existing file if open
            if self.log_file:
                self.log_file.close()
            
            # Open new log file
            self.log_file = open(file_path, 'a')
            self.log_file_path = file_path
            
            # Add header to log file
            header = f"=== Keyswitch Tester Log Started at {QDateTime.currentDateTime().toString('yyyy-MM-dd HH:mm:ss')} ===\n"
            self.log_file.write(header)
            self.log_file.flush()
            
            # Add message to display
            self.text_display.append(f"[INFO] Logging to {file_path}")
            return True
        except Exception as e:
            self.text_display.append(f"[ERROR] Failed to open log file: {str(e)}")
            return False
    
    def restart_auto_logging(self):
        """Restart logging if there was an error"""
        if self.log_file:
            try:
                self.log_file.close()
            except Exception:
                pass
            self.log_file = None
        
        # Try to restart logging
        self.start_auto_logging()
    
    def save_log(self):
        """Save the current log content to a file"""
        # Default filename with timestamp
        default_filename = f"keyswitch_log_{QDateTime.currentDateTime().toString('yyyyMMdd_HHmmss')}.txt"
        
        # Open file dialog
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Log File",
            os.path.join(os.path.expanduser("~"), default_filename),
            "Text Files (*.txt);;All Files (*)"
        )
        
        if file_path:
            try:
                # Save log content to file
                with open(file_path, 'w') as f:
                    f.write(self.text_display.toPlainText())
                self.text_display.append(f"[INFO] Log saved to {file_path}")
            except Exception as e:
                self.text_display.append(f"[ERROR] Failed to save log: {str(e)}")
    
    def clear(self):
        """Clear the log"""
        self.text_display.clear()
        
        # Add a message to the log file if log file is open
        if self.log_file:
            try:
                timestamp = QDateTime.currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                self.log_file.write(f"[{timestamp}] === Log cleared ===\n")
                self.log_file.flush()
            except Exception:
                pass

class StationStatusWidget(QWidget):
    # Signal emitted when a station's enabled state changes
    station_state_changed = Signal(int, bool)  # station_id, enabled
    
    def __init__(self):
        super().__init__()
        
        # Main layout with smaller margins
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(8)  # Reduce spacing between header and stations
        
        # Header row with minimal height - using a simple horizontal layout
        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(10, 0, 10, 0)  # No vertical padding
        header_layout.setSpacing(0)
        
        # Empty first column to align with station numbers
        header_layout.addWidget(QLabel(""))  # Empty placeholder
        
        # Column headers with consistent styling - only the data headers
        headers = ["Cycle Count", "Failure Count", "Current (A)", "Enabled"]
        for header_text in headers:
            label = QLabel(header_text)
            label.setFont(QFont("Arial", 9, QFont.Bold))
            label.setAlignment(Qt.AlignCenter)
            label.setStyleSheet("padding: 0px; margin: 0px;")  # No padding within label
            header_layout.addWidget(label)
        
        # Add header layout directly to main layout without a container frame
        main_layout.addLayout(header_layout)
        
        # Add a minimal spacer below the headers
        main_layout.addSpacing(2)
        
        # Station rows container
        stations_container = QWidget()
        stations_layout = QVBoxLayout(stations_container)
        stations_layout.setContentsMargins(0, 0, 0, 0)
        stations_layout.setSpacing(5)
        
        # Value labels for each station
        self.value_labels = {}
        self.toggle_sliders = {}
        
        # Create each station row
        for station_idx in range(1, 5):
            # Station row frame
            station_frame = QFrame()
            station_frame.setFrameShape(QFrame.StyledPanel)
            station_frame.setStyleSheet("background-color: white; border: 1px solid #e0e0e0; border-radius: 4px;")
            
            # Horizontal layout for station values
            station_layout = QHBoxLayout(station_frame)
            station_layout.setContentsMargins(8, 8, 8, 8)
            
            # Station number with distinctive styling
            station_label = QLabel(f"Station {station_idx}")
            station_label.setStyleSheet("color: #2196F3; font-weight: bold;")
            station_label.setAlignment(Qt.AlignCenter)
            station_layout.addWidget(station_label)
            
            # Value labels with consistent styling
            cycle_count_label = QLabel("0")
            failure_count_label = QLabel("0")
            current_label = QLabel("0.00")
            
            # Apply consistent styling to all value labels
            for label in [cycle_count_label, failure_count_label, current_label]:
                label.setAlignment(Qt.AlignCenter)
                label.setStyleSheet("padding: 2px;")
            
            # Add labels to layout
            station_layout.addWidget(cycle_count_label)
            station_layout.addWidget(failure_count_label)
            station_layout.addWidget(current_label)
            
            # Add toggle slider
            toggle_slider = QSlider(Qt.Horizontal)
            toggle_slider.setMinimum(0)
            toggle_slider.setMaximum(1)
            toggle_slider.setValue(1)  # Start enabled
            toggle_slider.setFixedWidth(60)
            toggle_slider.setStyleSheet("""
                QSlider::groove:horizontal {
                    border: 1px solid #999999;
                    height: 8px;
                    background: #e0e0e0;
                    margin: 2px 0;
                    border-radius: 4px;
                }
                QSlider::handle:horizontal {
                    background: #2196F3;
                    border: 1px solid #5c5c5c;
                    width: 18px;
                    margin: -2px 0;
                    border-radius: 9px;
                }
                QSlider::handle:horizontal:disabled {
                    background: #cccccc;
                }
            """)
            
            # Connect slider to signal
            toggle_slider.valueChanged.connect(
                lambda value, idx=station_idx: self.station_state_changed.emit(idx, bool(value))
            )
            
            station_layout.addWidget(toggle_slider)
            
            # Store references to the labels and slider
            self.value_labels[station_idx] = {
                'cycle_count': cycle_count_label,
                'failure_count': failure_count_label,
                'current': current_label
            }
            self.toggle_sliders[station_idx] = toggle_slider
            
            # Add to stations layout
            stations_layout.addWidget(station_frame)
        
        main_layout.addWidget(stations_container)
        self.setLayout(main_layout)
        
        # Set fixed width with some margin for padding
        self.setFixedWidth(450)  # Increased width to accommodate toggle slider
    
    def update_station_value(self, station_id, value_type, value):
        """Update a specific value for a station"""
        if 1 <= station_id <= 4 and value_type in self.value_labels[station_id]:
            # Format current values with 2 decimal places
            if value_type == 'current' and isinstance(value, (int, float)):
                display_value = f"{value:.2f}"
            else:
                display_value = str(value)
            self.value_labels[station_id][value_type].setText(display_value)
    
    def set_station_enabled(self, station_id, enabled):
        """Set the enabled state of a station's toggle slider"""
        if 1 <= station_id <= 4:
            self.toggle_sliders[station_id].setValue(1 if enabled else 0)

class ControlWidget(QWidget):
    def __init__(self, on_toggle):
        super().__init__()
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        
        # Clean, simplified styling
        button_style = """
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                padding: 10px;
                font-size: 14px;
                border-radius: 3px;
                min-height: 40px;
            }
            QPushButton:checked {
                background-color: #f44336;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:checked:hover {
                background-color: #e53935;
            }
        """
        
        # Start/stop button
        self.start_stop_btn = QPushButton("Start")
        self.start_stop_btn.setCheckable(True)
        self.start_stop_btn.clicked.connect(on_toggle)
        self.start_stop_btn.setStyleSheet(button_style)
        
        layout.addWidget(self.start_stop_btn)
        self.setLayout(layout)
    
    def update_state(self, is_running):
        self.start_stop_btn.setChecked(is_running)
        self.start_stop_btn.setText("Stop" if is_running else "Start")
    
    def set_enabled(self, enabled):
        self.start_stop_btn.setEnabled(enabled) 