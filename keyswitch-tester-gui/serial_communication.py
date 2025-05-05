import serial
import serial.tools.list_ports
from constants import SERIAL_BAUDRATE, SERIAL_TIMEOUT

class SerialManager:
    def __init__(self):
        self.serial_port = None
        self.serial_ports = []

    def get_available_ports(self):
        """Get list of available serial ports"""
        return [port.device for port in serial.tools.list_ports.comports()]

    def connect(self, port_name):
        """Connect to the specified serial port"""
        if self.serial_port is not None:
            self.serial_port.close()
        
        if port_name:
            try:
                self.serial_port = serial.Serial(
                    port_name,
                    baudrate=SERIAL_BAUDRATE,
                    timeout=SERIAL_TIMEOUT
                )
                return True
            except serial.SerialException:
                return False
        return False

    def disconnect(self):
        """Disconnect from the current serial port"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.serial_port = None

    def write(self, data):
        """Write data to the serial port"""
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.write(data)
                return True
            except serial.SerialException:
                return False
        return False

    def read_line(self):
        """Read a line from the serial port"""
        if self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    return self.serial_port.readline().decode().strip()
            except serial.SerialException:
                return None
        return None

    def parse_current_report(self, line):
        """Parse a current report message from the serial port
        Format: CURRENT:station:value
        Returns: (station_idx, current_value) or None if invalid format
        """
        if not line.startswith("CURRENT:"):
            return None
        
        try:
            parts = line.split(":")
            if len(parts) != 3:
                return None
                
            station = int(parts[1])
            current = float(parts[2])
            return (station, current)
        except (ValueError, IndexError):
            return None

    def parse_event_message(self, line):
        """Parse an event message from the serial port
        Format: EVENT:message
        Returns: message string or None if invalid format
        """
        if not line.startswith("EVENT:"):
            return None
            
        try:
            return line[6:]  # Remove "EVENT:" prefix
        except IndexError:
            return None 