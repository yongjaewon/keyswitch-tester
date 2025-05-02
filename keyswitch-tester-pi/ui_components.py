import tkinter as tk
from tkinter import ttk
import time
import threading

class KeyswitchTesterUI:
    def __init__(self, root, tester, event_processor):
        self.root = root
        self.tester = tester
        self.event_processor = event_processor
        
        # Configure style
        style = ttk.Style()
        style.configure("TButton", padding=5)
        style.configure("TLabel", padding=5)
        
        # Create main frame
        self.main_frame = ttk.Frame(root, padding="10")
        self.main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Status label
        self.status_label = ttk.Label(self.main_frame, text="Status: Connecting...")
        self.status_label.grid(row=0, column=0, columnspan=2, sticky=tk.W)
        
        # Buttons
        ttk.Button(self.main_frame, text="Start", command=self.start).grid(row=1, column=0, padx=5, pady=5)
        ttk.Button(self.main_frame, text="Stop", command=self.stop).grid(row=1, column=1, padx=5, pady=5)
        
        # Station counts and reset buttons
        self.station_labels = []
        self.reset_buttons = []
        for i in range(4):
            # Create a frame for each station
            station_frame = ttk.Frame(self.main_frame)
            station_frame.grid(row=2+i, column=0, columnspan=2, sticky=tk.W, pady=5)
            
            # Station label
            label = ttk.Label(station_frame, text=f"Station {i}: 0 failures")
            label.grid(row=0, column=0, sticky=tk.W)
            self.station_labels.append(label)
            
            # Reset button for this station
            reset_btn = ttk.Button(station_frame, text=f"Reset Station {i}", 
                                  command=lambda station=i: self.reset_station(station))
            reset_btn.grid(row=0, column=1, padx=10)
            self.reset_buttons.append(reset_btn)
            
        # Event log frame
        event_frame = ttk.LabelFrame(self.main_frame, text="Event Log", padding="5")
        event_frame.grid(row=6, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        
        # Event log text widget with scrollbar
        self.event_text = tk.Text(event_frame, height=8, width=50, wrap=tk.WORD)
        scrollbar = ttk.Scrollbar(event_frame, orient=tk.VERTICAL, command=self.event_text.yview)
        self.event_text.configure(yscrollcommand=scrollbar.set)
        
        self.event_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Make the event log read-only
        self.event_text.config(state=tk.DISABLED)
        
        # Configure grid weights for resizing
        self.main_frame.columnconfigure(0, weight=1)
        self.main_frame.rowconfigure(6, weight=1)
        event_frame.columnconfigure(0, weight=1)
        event_frame.rowconfigure(0, weight=1)
        
        # Start update thread
        self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
        self.update_thread.start()
        
        # Auto-connect to Arduino
        self.auto_connect()
        
        # Add a test event to verify the log is working
        self.add_event_to_log("Application started - waiting for events")
        
    def add_event_to_log(self, event_message):
        """Add an event message to the event log"""
        try:
            # Use a more efficient approach to update the text widget
            timestamp = time.strftime("%H:%M:%S")
            log_entry = f"[{timestamp}] {event_message}\n"
            
            # Update the text widget in a single operation
            self.event_text.config(state=tk.NORMAL)
            self.event_text.insert(tk.END, log_entry)
            self.event_text.see(tk.END)  # Scroll to the end
            self.event_text.config(state=tk.DISABLED)
            
            # No need to force update here as we do it in process_events
        except Exception as e:
            print(f"Error adding event to log: {str(e)}")
            
    def auto_connect(self):
        # Try to connect to Arduino
        success, message = self.tester.connect_to_arduino()
        if success:
            self.status_label.config(text=f"Status: {message}")
            self.add_event_to_log(f"Connected to Arduino: {message}")
        else:
            # If connection fails, try again after a delay
            self.status_label.config(text=f"Status: {message} - Retrying in 5 seconds...")
            self.add_event_to_log(f"Connection failed: {message} - Retrying in 5 seconds...")
            self.root.after(5000, self.auto_connect)
            
    def start(self):
        if self.tester.is_connected:
            # Update UI immediately
            self.status_label.config(text="Status: Starting...")
            self.add_event_to_log("Starting system...")
            
            # Use a separate thread for the serial communication
            threading.Thread(target=self._start_thread, daemon=True).start()
    
    def _start_thread(self):
        """Thread function for start command"""
        response = self.tester.send_command("START")
        # Use after() to update UI from the main thread
        if response == "STATUS:RUNNING":
            self.root.after(0, lambda: self._start_success())
        elif response:
            self.root.after(0, lambda: self._start_failure(response))
    
    def _start_success(self):
        """Update UI after successful start"""
        self.tester.is_running = True
        self.status_label.config(text="Status: Running")
        self.add_event_to_log("System started")
    
    def _start_failure(self, response):
        """Update UI after failed start"""
        self.add_event_to_log(f"Start command failed: {response}")

    def stop(self):
        if self.tester.is_connected:
            # Update UI immediately
            self.status_label.config(text="Status: Stopping...")
            self.add_event_to_log("Stopping system...")
            
            # Use a separate thread for the serial communication
            threading.Thread(target=self._stop_thread, daemon=True).start()
    
    def _stop_thread(self):
        """Thread function for stop command"""
        response = self.tester.send_command("STOP")
        # Use after() to update UI from the main thread
        if response == "STOPPED":
            self.root.after(0, lambda: self._stop_success())
        elif response:
            self.root.after(0, lambda: self._stop_failure(response))
    
    def _stop_success(self):
        """Update UI after successful stop"""
        self.tester.is_running = False
        self.status_label.config(text="Status: Connected")
        self.add_event_to_log("System stopped")
    
    def _stop_failure(self, response):
        """Update UI after failed stop"""
        self.add_event_to_log(f"Stop command failed: {response}")

    def reset_station(self, station):
        if self.tester.is_connected:
            # Update UI immediately
            self.add_event_to_log(f"Resetting station {station}...")
            
            # Use a separate thread for the serial communication
            threading.Thread(target=lambda: self._reset_station_thread(station), daemon=True).start()
    
    def _reset_station_thread(self, station):
        """Thread function for reset station command"""
        response = self.tester.send_command(f"RESET_STATION:{station}")
        # Use after() to update UI from the main thread
        if response == "RESET_DONE":
            self.root.after(0, lambda: self._reset_station_success(station))
        elif response:
            self.root.after(0, lambda: self._reset_station_failure(station, response))
    
    def _reset_station_success(self, station):
        """Update UI after successful station reset"""
        self.station_labels[station].config(text=f"Station {station}: 0 failures")
        self.add_event_to_log(f"Reset station {station}")
    
    def _reset_station_failure(self, station, response):
        """Update UI after failed station reset"""
        self.add_event_to_log(f"Reset station {station} failed: {response}")
        
    def update_counts(self):
        if self.tester.is_connected:
            response = self.tester.send_command("COUNTS")
            if response and response.startswith("FAIL_COUNT:"):
                parts = response.split(":")
                if len(parts) == 3:
                    station = int(parts[1])
                    count = int(parts[2])
                    self.station_labels[station].config(text=f"Station {station}: {count} failures")
                    
    def update_loop(self):
        while True:
            try:
                if self.tester.is_connected:
                    # Read events regardless of running state
                    self.tester.read_events()
                    
                    # Update counts only when running
                    if self.tester.is_running:
                        self.update_counts()
                time.sleep(0.001)  # Minimal sleep to prevent CPU hogging
            except Exception as e:
                print(f"Error in update loop: {str(e)}")
                time.sleep(0.05)  # Short sleep on error 