from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, 
                             QPushButton, QLabel, QTextEdit, QFileDialog,
                             QGridLayout, QFrame, QSlider, QCheckBox, QAbstractButton, QMessageBox)
from PySide6.QtCore import QDateTime, Qt, Signal, Property, QRect, QSize, QPropertyAnimation, QEasingCurve
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QPaintEvent
import os
from constants import (
    START_BUTTON_STYLE, RESET_BUTTON_STYLE, LOG_BUTTON_STYLE, 
    TOGGLE_BUTTON_STYLE, STATION_FRAME_STYLE, STATION_LABEL_STYLE,
    COLOR_SUCCESS, COLOR_ERROR, COLOR_TEXT_LIGHT
)

class ToggleSwitch(QAbstractButton):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # Customize appearance
        self.setCheckable(True)
        self.setChecked(True)
        
        # Set fixed size
        self.setFixedSize(60, 30)
        
        # Customize cursor
        self.setCursor(Qt.PointingHandCursor)
        
        # Internal state
        self._track_color_on = QColor("#4CAF50")  # Green
        self._track_color_off = QColor("#e0e0e0") # Gray
        self._thumb_color_on = QColor("white")
        self._thumb_color_off = QColor("white")
        self._track_opacity = 0.6
        
        # Setup animation
        self._thumb_position = 1.0  # Start as ON
    
    def sizeHint(self):
        return QSize(60, 30)
    
    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._thumb_position = 1.0 if self.isChecked() else 0.0
    
    def paintEvent(self, event: QPaintEvent):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        
        # Calculate sizes
        w, h = self.width(), self.height()
        margin = max(3, int(h * 0.1))
        
        # Draw track
        track_brush = QBrush(self._track_color_on if self.isChecked() else self._track_color_off)
        track_pen = QPen(track_brush.color().darker(110) if self.isChecked() else track_brush.color().darker(150))
        track_pen.setWidth(1)
        
        p.setPen(track_pen)
        p.setBrush(track_brush)
        p.drawRoundedRect(0, 0, w, h, h//2, h//2)
        
        # Calculate thumb position
        thumb_width = h - 2 * margin
        pos = margin + (w - thumb_width - 2 * margin) * self._thumb_position
        
        # Draw thumb
        thumb_brush = QBrush(self._thumb_color_on if self.isChecked() else self._thumb_color_off)
        thumb_pen = QPen(QColor("#999999") if not self.isChecked() else QColor("#45a049"))
        thumb_pen.setWidth(1)
        
        p.setPen(thumb_pen)
        p.setBrush(thumb_brush)
        p.drawEllipse(int(pos), margin, thumb_width, thumb_width)
        
        p.end()
    
    def setChecked(self, checked):
        super().setChecked(checked)
        self._thumb_position = 1.0 if checked else 0.0
        self.update()
    
    def mouseReleaseEvent(self, event):
        super().mouseReleaseEvent(event)
        if event.button() == Qt.LeftButton:
            self.setChecked(not self.isChecked())
            self.clicked.emit()

class SerialLogWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.setup_ui()
        self.start_auto_logging()
    
    def setup_ui(self):
        """Set up the user interface components"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(3)
        
        # Header with controls
        header_layout = QHBoxLayout()
        
        # Title
        log_title = QLabel("Log")
        log_title.setFont(QFont("Arial", 9, QFont.Bold))
        header_layout.addWidget(log_title)
        header_layout.addStretch()
        
        # Clear button (Save button removed)
        clear_btn = QPushButton("Clear")
        clear_btn.setStyleSheet(LOG_BUTTON_STYLE)
        clear_btn.clicked.connect(self.clear)
        header_layout.addWidget(clear_btn)
        
        layout.addLayout(header_layout)
        
        # Text display
        self.text_display = QTextEdit()
        self.text_display.setReadOnly(True)
        self.text_display.setStyleSheet("background-color: white; border: 1px solid #e0e0e0; border-radius: 3px;")
        self.text_display.setMaximumHeight(160)
        layout.addWidget(self.text_display)
        
        # Initialize logging variables
        self.log_file = None
        self.log_file_path = None
    
    def append_message(self, message):
        """Append a message to the log"""
        timestamp = QDateTime.currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
        formatted_message = f"[{timestamp}] {message}"
        
        # Add to display
        self.text_display.append(formatted_message)
        
        # Save to file if log file is open
        if self.log_file:
            try:
                self.log_file.write(formatted_message + "\n")
                self.log_file.flush()
            except Exception as e:
                self.text_display.append(f"[ERROR] Failed to write to log file: {str(e)}")
                self.restart_auto_logging()
    
    def start_auto_logging(self):
        """Automatically start logging to a file in the logs directory"""
        logs_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "logs")
        os.makedirs(logs_dir, exist_ok=True)
        
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
        
        self.start_auto_logging()
    
    def clear(self):
        """Clear the log"""
        self.text_display.clear()
        
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
    
    # Signal emitted when a reset is requested
    reset_requested = Signal(int, str)  # station_id, field_type ('cycle_count' or 'failure_count')
    
    def __init__(self):
        super().__init__()
        
        # Define column structure
        self.columns = [
            {"name": "Station", "width": 90},
            {"name": "Cycle Count", "width": 110},
            {"name": "Failures", "width": 70},
            {"name": "KSwitch (A)", "width": 90},  # Renamed from "Current (A)" to "KSwitch (A)"
            {"name": "Starter (A)", "width": 90},  # Added column for starter current
            {"name": "Enabled", "width": 65}
        ]
        
        # Main layout
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(3, 3, 3, 3)  # Reduced from 5,5,5,5 to 3,3,3,3
        main_layout.setSpacing(3)  # Reduced from 4 to 3
        
        # Create header
        header = self._create_header()
        main_layout.addWidget(header)
        
        # Create station rows
        stations_container = QWidget()
        stations_layout = QVBoxLayout(stations_container)
        stations_layout.setContentsMargins(0, 0, 0, 0)
        stations_layout.setSpacing(5)  # Increased from 3 to 5
        
        # Storage for UI references
        self.value_labels = {}
        self.toggle_buttons = {}
        
        # Create each station row
        for station_id in range(1, 5):
            row = self._create_station_row(station_id)
            stations_layout.addWidget(row)
        
        main_layout.addWidget(stations_container)
        
        # Set width based on columns
        total_width = sum(col["width"] for col in self.columns) + 40
        self.setFixedWidth(total_width)
    
    def _create_header(self):
        """Create the column headers"""
        header_widget = QWidget()
        header_layout = QGridLayout(header_widget)
        header_layout.setContentsMargins(10, 2, 10, 0)  # Reduced bottom margin to 0
        header_layout.setHorizontalSpacing(5)
        header_layout.setVerticalSpacing(0)
        
        # Add headers
        for col, column in enumerate(self.columns):
            # Skip the first column ("Station") and last column ("Enabled") to remove their header text
            if col == 0 or col == 5:  # First and last columns
                # Create an empty label to maintain spacing
                label = QLabel("")
                label.setMinimumWidth(column["width"])
                header_layout.addWidget(label, 0, col)
                continue
                
            label = QLabel(column["name"])
            label.setFont(QFont("Arial", 9, QFont.Bold))
            label.setAlignment(Qt.AlignBottom | Qt.AlignHCenter)
            label.setMinimumWidth(column["width"])
            label.setStyleSheet("padding: 0px 0px 3px 0px; margin: 0px;")
            header_layout.addWidget(label, 0, col)
        
        return header_widget
    
    def _create_station_row(self, station_id):
        """Create a row for a station"""
        # Create frame
        frame = QFrame()
        frame.setFrameShape(QFrame.StyledPanel)
        frame.setStyleSheet(STATION_FRAME_STYLE)
        
        # Create layout
        layout = QGridLayout(frame)
        layout.setContentsMargins(8, 8, 8, 8)  # Fully restored to original 8px on all sides
        layout.setHorizontalSpacing(5)
        layout.setVerticalSpacing(0)
        
        # Disable column stretching
        for i in range(len(self.columns)):
            layout.setColumnStretch(i, 0)
            layout.setColumnMinimumWidth(i, self.columns[i]["width"])
        
        # Station label
        station_label = QLabel(f"Station {station_id}")
        station_label.setStyleSheet(STATION_LABEL_STYLE)
        station_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(station_label, 0, 0)
        
        # Value labels
        cycle_count = ClickableLabel("0")
        cycle_count.setObjectName(f"cycle_count_{station_id}")
        cycle_count.clicked.connect(lambda: self._handle_reset_click(station_id, "cycle_count"))
        
        failure_count = ClickableLabel("0")
        failure_count.setObjectName(f"failure_count_{station_id}")
        failure_count.clicked.connect(lambda: self._handle_reset_click(station_id, "failure_count"))
        
        keyswitch_current = QLabel("0.00")
        starter_current = QLabel("0.00")  # Added label for starter current
        
        for label in [cycle_count, failure_count, keyswitch_current, starter_current]:  # Added starter_current to the list
            label.setAlignment(Qt.AlignCenter)
            label.setStyleSheet("padding: 2px;")
        
        # Make clickable labels show a hand cursor to indicate they're interactive
        cycle_count.setCursor(Qt.PointingHandCursor)
        failure_count.setCursor(Qt.PointingHandCursor)
        
        layout.addWidget(cycle_count, 0, 1)
        layout.addWidget(failure_count, 0, 2)
        layout.addWidget(keyswitch_current, 0, 3)
        layout.addWidget(starter_current, 0, 4)  # Added starter current label to grid
        
        # Toggle button
        toggle = QPushButton("ON")
        toggle.setCheckable(True)
        toggle.setChecked(True)
        toggle.setFixedSize(60, 35)  # Restore to original size
        toggle.setCursor(Qt.PointingHandCursor)
        toggle.setStyleSheet(TOGGLE_BUTTON_STYLE)
        
        # Connect signal with a closure to capture the station_id
        toggle.clicked.connect(lambda _, s=station_id: self._handle_toggle_change(s))
        
        layout.addWidget(toggle, 0, 5, Qt.AlignCenter)  # Changed column from 4 to 5
        
        # Store references
        self.value_labels[station_id] = {
            'cycle_count': cycle_count,
            'failure_count': failure_count,
            'current': keyswitch_current,  # Keep 'current' for backwards compatibility
            'keyswitch_current': keyswitch_current,  # Add a more specific reference
            'starter_current': starter_current  # Add starter current reference
        }
        self.toggle_buttons[station_id] = toggle
        
        return frame
    
    def _handle_toggle_change(self, station_id):
        """Handle toggle button state change"""
        btn = self.toggle_buttons[station_id]
        is_checked = btn.isChecked()
        btn.setText("ON" if is_checked else "OFF")
        self.station_state_changed.emit(station_id, is_checked)
    
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
        """Set the enabled state of a station's toggle button"""
        if 1 <= station_id <= 4:
            button = self.toggle_buttons[station_id]
            button.setChecked(enabled)
            button.setText("ON" if enabled else "OFF")
    
    def _handle_reset_click(self, station_id, field_type):
        """Handle clicks on cycle count or failure count fields"""
        field_name = "Cycle Count" if field_type == "cycle_count" else "Failure Count"
        
        # Create and configure message box
        msg_box = QMessageBox()
        msg_box.setWindowTitle("Confirm Reset")
        msg_box.setText(f"Are you sure you want to reset the {field_name} for Station {station_id}?")
        msg_box.setIcon(QMessageBox.Question)
        msg_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        msg_box.setDefaultButton(QMessageBox.No)
        
        # Show dialog and get result
        result = msg_box.exec()
        
        # Process result
        if result == QMessageBox.Yes:
            # Emit signal to reset the field
            self.reset_requested.emit(station_id, field_type)

class ClickableLabel(QLabel):
    """A label that emits a clicked signal when clicked"""
    clicked = Signal()
    
    def __init__(self, text=""):
        super().__init__(text)
    
    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit()
        super().mousePressEvent(event)

class ControlWidget(QWidget):
    def __init__(self, on_toggle):
        super().__init__()
        self.setup_ui(on_toggle)
    
    def setup_ui(self, on_toggle):
        """Set up the user interface"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 2, 5, 2)
        
        # Start/stop button
        self.start_stop_btn = QPushButton("Start")
        self.start_stop_btn.setCheckable(True)
        self.start_stop_btn.clicked.connect(on_toggle)
        self.start_stop_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLOR_SUCCESS};
                color: {COLOR_TEXT_LIGHT};
                border: none;
                padding: 6px;
                font-size: 14px;
                border-radius: 4px;
                min-height: 32px;
            }}
        """)
        
        layout.addWidget(self.start_stop_btn)
    
    def update_state(self, is_running):
        """Update the button state"""
        self.start_stop_btn.setChecked(is_running)
        self.start_stop_btn.setText("Stop" if is_running else "Start")
        self.start_stop_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLOR_ERROR if is_running else COLOR_SUCCESS};
                color: {COLOR_TEXT_LIGHT};
                border: none;
                padding: 6px;
                font-size: 14px;
                border-radius: 4px;
                min-height: 32px;
            }}
        """)
    
    def set_enabled(self, enabled):
        """Enable or disable the button"""
        self.start_stop_btn.setEnabled(enabled) 