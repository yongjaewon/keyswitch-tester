import serial
import serial.tools.list_ports
import time
import queue

class KeyswitchTesterApp:
    def __init__(self):
        self.serial_port = None
        self.is_connected = False
        self.is_running = False
        self.station_counts = [0] * 4  # Assuming 4 stations based on the code
        self.station_enabled = [True] * 4  # Initialize all stations as enabled
        self.event_queue = queue.Queue()
        self.debug_mode = True  # Enable debug mode
        self.last_read_time = time.time()
        self.read_timeout = 0.5  # Timeout for read operations in seconds
        self.buffer_size = 1024  # Buffer size for reading data
        self.response_received = False
        self.command_response = ""
        self.waiting_for_command = None

    def connect_to_arduino(self):
        try:
            # Find Arduino port
            ports = list(serial.tools.list_ports.comports())
            arduino_port = None
            for port in ports:
                if "OpenRB-150" in port.description:
                    arduino_port = port.device
                    break
            
            if arduino_port:
                # Configure serial port with better settings
                self.serial_port = serial.Serial(
                    port=arduino_port,
                    baudrate=115200,
                    timeout=0.1,  # Shorter timeout for more responsive reading
                    write_timeout=1.0,
                    inter_byte_timeout=0.1,
                    rtscts=False,
                    dsrdtr=False
                )
                self.is_connected = True
                return True, f"Connected to {arduino_port}"
            else:
                return False, "Arduino not found"
        except Exception as e:
            return False, f"Connection error: {str(e)}"

    def send_command(self, command, timeout=2.0):
        if not self.serial_port or not self.is_connected:
            return "Not connected"

        try:
            # Clear any old incoming data before sending
            self.serial_port.reset_input_buffer()

            # Send the command
            self.serial_port.write(f"{command}\n".encode())
            self.serial_port.flush()  # Ensure data is sent

            # Wait for response using a more reliable approach
            start_time = time.time()
            response_received = False
            response = ""
            
            # Create a flag to track if we've received the response
            self.response_received = False
            self.command_response = ""
            
            # Set a flag to indicate we're waiting for a specific command response
            self.waiting_for_command = command
            
            # Wait for the response to be processed by read_events
            while time.time() - start_time < timeout and not self.response_received:
                time.sleep(0.01)  # Short sleep to avoid busy-waiting
                
            # Check if we got a response
            if self.response_received:
                response = self.command_response
                # Reset the flags
                self.response_received = False
                self.waiting_for_command = None
                self.command_response = ""
                return response
            else:
                # Reset the flags
                self.waiting_for_command = None
                return ""

        except Exception as e:
            if self.debug_mode:
                print(f"Error in send_command: {str(e)}")
            # Reset the flags
            self.waiting_for_command = None
            return f"Error: {str(e)}"
            
    def get_station_enabled(self):
        """Get the enabled state of all stations"""
        response = self.send_command("GET_STATION_ENABLED")
        if response.startswith("STATION_ENABLED:"):
            states = response[15:].strip().split(',')
            self.station_enabled = [state == "1" for state in states]
        return self.station_enabled

    def set_station_enabled(self, station_index, enabled):
        """Set the enabled state of a specific station"""
        if 0 <= station_index < len(self.station_enabled):
            command = f"SET_STATION_ENABLED:{station_index}:{'1' if enabled else '0'}"
            response = self.send_command(command)
            if response.startswith("STATION_ENABLED:"):
                states = response[15:].strip().split(',')
                self.station_enabled = [state == "1" for state in states]
            return True
        return False

    def read_events(self):
        """Read any available events from the serial port"""
        if not self.serial_port or not self.is_connected:
            return
            
        try:
            # Read all available data without frequency limiting
            if self.serial_port.in_waiting:
                # Read in chunks to avoid blocking
                data = self.serial_port.read(self.serial_port.in_waiting).decode('utf-8', errors='replace')
                
                # Process the data line by line
                for line in data.splitlines():
                    line = line.strip()
                    if not line:
                        continue
                        
                    if self.debug_mode:
                        print(f"Raw serial data: {line}")
                    
                    # Handle REQUEST_STATION_ENABLED from Arduino
                    if line == "REQUEST_STATION_ENABLED":
                        self.send_station_enabled_states()
                        continue
                    
                    # Check if this is a response to a command we're waiting for
                    if hasattr(self, 'waiting_for_command') and self.waiting_for_command:
                        # If we're waiting for a specific command response, check if this line matches
                        if line == "STATUS:RUNNING" and "START" in self.waiting_for_command:
                            self.response_received = True
                            self.command_response = line
                            continue
                        elif line == "STOPPED" and "STOP" in self.waiting_for_command:
                            self.response_received = True
                            self.command_response = line
                            continue
                        elif line == "RESET_DONE" and "RESET_STATION" in self.waiting_for_command:
                            self.response_received = True
                            self.command_response = line
                            continue
                        elif line.startswith("FAIL_COUNT:") and "COUNTS" in self.waiting_for_command:
                            self.response_received = True
                            self.command_response = line
                            continue
                        elif line.startswith("STATION_ENABLED:") and "GET_STATION_ENABLED" in self.waiting_for_command:
                            self.response_received = True
                            self.command_response = line
                            continue
                    
                    # Update station enabled states if we receive a STATION_ENABLED message
                    if line.startswith("STATION_ENABLED:"):
                        states = line[15:].strip().split(',')
                        self.station_enabled = [state == "1" for state in states]
                    
                    # Add all non-empty lines to the event queue
                    if line:
                        if self.debug_mode:
                            print(f"Adding event to queue: {line}")
                        self.event_queue.put(line)
                        
        except Exception as e:
            print(f"Error reading events: {str(e)}")
            # Try to recover from serial errors
            try:
                self.serial_port.reset_input_buffer()
            except:
                pass
    
    def send_station_enabled_states(self):
        """Send the current station enabled states to the Arduino"""
        if self.serial_port and self.is_connected:
            try:
                # Format: STATION_ENABLED:1,1,1,1
                response = "STATION_ENABLED:" + ",".join(["1" if enabled else "0" for enabled in self.station_enabled])
                self.serial_port.write(f"{response}\n".encode())
                self.serial_port.flush()
                if self.debug_mode:
                    print(f"Sent station enabled states: {response}")
            except Exception as e:
                print(f"Error sending station enabled states: {str(e)}") 