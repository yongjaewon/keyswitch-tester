import serial
import serial.tools.list_ports
import time
import os
from constants import SERIAL_BAUDRATE, SERIAL_TIMEOUT

class SerialManager:
    def __init__(self):
        self.serial_port = None
        self.serial_ports = []

    def get_available_ports(self):
        """Get list of available serial ports"""
        try:
            return [port.device for port in serial.tools.list_ports.comports()]
        except Exception:
            return []
            
    def get_openrb_port(self):
        """Find a port with 'OpenRB' in the name or description"""
        try:
            for port in serial.tools.list_ports.comports():
                # Check device, name, description, hwid, etc. for "OpenRB"
                port_info = f"{port.device} {port.name} {port.description} {port.hwid}".lower()
                if "openrb" in port_info:
                    return port.device
                    
            # If no port with OpenRB in name is found, default to /dev/ttyACM0 if it exists
            if os.path.exists("/dev/ttyACM0"):
                return "/dev/ttyACM0"
                
            return None
        except Exception:
            return None

    def connect(self, port_name=None):
        """Connect to the specified serial port or auto-detect OpenRB"""
        if self.serial_port is not None:
            try:
                self.serial_port.close()
            except Exception:
                pass
            self.serial_port = None
        
        # If no port specified, try to find OpenRB
        if not port_name:
            port_name = self.get_openrb_port()
            
        if not port_name:
            return False
            
        try:
            # Check if port exists
            if not os.path.exists(port_name):
                return False
                
            self.serial_port = serial.Serial(
                port_name,
                baudrate=SERIAL_BAUDRATE,
                timeout=SERIAL_TIMEOUT
            )
            
            # Give the serial connection time to stabilize
            time.sleep(0.5)
            
            # Flush any pending data
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()
            
            return self.serial_port.is_open
            
        except Exception:
            if self.serial_port:
                try:
                    self.serial_port.close()
                except Exception:
                    pass
            self.serial_port = None
            return False

    def disconnect(self):
        """Disconnect from the current serial port"""
        if self.serial_port:
            try:
                if self.serial_port.is_open:
                    self.serial_port.close()
            except Exception:
                pass
        self.serial_port = None

    def write(self, data):
        """Write data to the serial port"""
        if not self.serial_port:
            return False
            
        try:
            if not self.serial_port.is_open:
                return False
                
            bytes_written = self.serial_port.write(data)
            return bytes_written > 0
        except Exception:
            return False

    def read_line(self):
        """Read a line from the serial port"""
        if not self.serial_port:
            return None
            
        try:
            if not self.serial_port.is_open:
                return None
                
            if self.serial_port.in_waiting:
                line = self.serial_port.readline()
                return line.decode('utf-8', errors='replace').strip()
        except Exception:
            return None
        return None 