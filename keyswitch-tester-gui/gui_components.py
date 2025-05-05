from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, 
                             QPushButton, QLabel, QComboBox, QTextEdit)
from PySide6.QtCore import QTimer
from constants import (START_BUTTON_STYLE, RESET_BUTTON_STYLE, 
                      NUM_STATIONS, SERIAL_CHECK_INTERVAL)

class SerialLogWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout()
        
        # Header
        header = QHBoxLayout()
        header.addWidget(QLabel("Serial Log"))
        clear_btn = QPushButton("Clear")
        clear_btn.setStyleSheet(RESET_BUTTON_STYLE)
        clear_btn.clicked.connect(self.clear)
        header.addWidget(clear_btn)
        layout.addLayout(header)
        
        # Text display
        self.text_display = QTextEdit()
        self.text_display.setReadOnly(True)
        self.text_display.setMaximumHeight(150)
        layout.addWidget(self.text_display)
        
        self.setLayout(layout)
    
    def append_message(self, message):
        """Append a message to the log"""
        self.text_display.append(message)
    
    def clear(self):
        """Clear the log"""
        self.text_display.clear()

class StationWidget(QWidget):
    def __init__(self, station_idx, on_reset):
        super().__init__()
        self.station_idx = station_idx
        self.cycle_count = 0
        self.failure_count = 0
        
        layout = QVBoxLayout()
        
        # Station header
        header = QHBoxLayout()
        header.addWidget(QLabel(f"Station {station_idx+1}"))
        reset_btn = QPushButton("Reset")
        reset_btn.setStyleSheet(RESET_BUTTON_STYLE)
        reset_btn.clicked.connect(lambda: on_reset(station_idx))
        header.addWidget(reset_btn)
        layout.addLayout(header)
        
        # Cycle count
        cycle_layout = QHBoxLayout()
        cycle_layout.addWidget(QLabel("Cycles:"))
        self.cycle_label = QLabel("0")
        self.cycle_label.setObjectName(f"station_{station_idx}_cycle")
        cycle_layout.addWidget(self.cycle_label)
        layout.addLayout(cycle_layout)
        
        # Failure count
        failure_layout = QHBoxLayout()
        failure_layout.addWidget(QLabel("Failures:"))
        self.failure_label = QLabel("0")
        self.failure_label.setObjectName(f"station_{station_idx}_failure")
        failure_layout.addWidget(self.failure_label)
        layout.addLayout(failure_layout)
        
        self.setLayout(layout)
    
    def update_counts(self, cycle_count, failure_count):
        """Update both cycle and failure counts"""
        self.cycle_count = cycle_count
        self.failure_count = failure_count
        self.cycle_label.setText(str(cycle_count))
        self.failure_label.setText(str(failure_count))

class SerialPortWidget(QWidget):
    def __init__(self, on_refresh):
        super().__init__()
        layout = QHBoxLayout()
        
        self.port_combo = QComboBox()
        layout.addWidget(QLabel("Serial Port:"))
        layout.addWidget(self.port_combo)
        
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(on_refresh)
        layout.addWidget(refresh_btn)
        
        self.setLayout(layout)
    
    def update_ports(self, ports):
        self.port_combo.clear()
        self.port_combo.addItems(ports)
    
    def get_selected_port(self):
        return self.port_combo.currentText()

class ControlWidget(QWidget):
    def __init__(self, on_toggle):
        super().__init__()
        layout = QVBoxLayout()
        
        self.start_stop_btn = QPushButton("Start")
        self.start_stop_btn.setCheckable(True)
        self.start_stop_btn.clicked.connect(on_toggle)
        self.start_stop_btn.setStyleSheet(START_BUTTON_STYLE)
        
        layout.addWidget(self.start_stop_btn)
        self.setLayout(layout)
    
    def update_state(self, is_running):
        self.start_stop_btn.setChecked(is_running)
        self.start_stop_btn.setText("Stop" if is_running else "Start")
    
    def set_enabled(self, enabled):
        self.start_stop_btn.setEnabled(enabled) 