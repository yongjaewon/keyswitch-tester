import os
if not os.environ.get('DISPLAY'):
    os.environ['DISPLAY'] = ':0'

import tkinter as tk

def 
    # Create the main window
    root = tk.Tk()
    root.title("Keyswitch Tester Control")
    
    # Create the tester app
    tester = KeyswitchTesterApp()
    
    # Create the event processor
    event_processor = EventProcessor(tester, root, None)  # We'll set the add_event_to_log function later
    
    # Create the UI
    ui = KeyswitchTesterUI(root, tester, event_processor)
    
    # Set the add_event_to_log function for the event processor
    event_processor.add_event_to_log = ui.add_event_to_log
    
    # Start the event processor
    event_processor.start()
    
    # Start the main loop
    root.mainloop()
    
    # Stop the event processor when the application exits
    event_processor.stop()

if __name__ == "__main__":
    main() 