import time
import threading

class EventProcessor:
    def __init__(self, tester, root, add_event_to_log):
        self.tester = tester
        self.root = root
        self.add_event_to_log = add_event_to_log
        self.is_running = True
        
    def start(self):
        """Start the event processing thread"""
        self.process_thread = threading.Thread(target=self.process_events, daemon=True)
        self.process_thread.start()
        
    def stop(self):
        """Stop the event processing thread"""
        self.is_running = False
        
    def process_events(self):
        """Process events from the queue"""
        while self.is_running:
            try:
                # Process all available events without limiting
                events_processed = 0
                while True:
                    try:
                        event = self.tester.event_queue.get_nowait()
                        if self.tester.debug_mode:
                            print(f"Processing event from queue: {event}")
                        
                        # Add event to log directly
                        self.add_event_to_log(event)
                        events_processed += 1
                        
                        # Safety check to prevent infinite loops
                        if events_processed > 100:
                            break
                    except:
                        break
                        
                # If we processed events, force an immediate UI update
                if events_processed > 0:
                    self.root.update_idletasks()
                    
            except Exception as e:
                print(f"Error processing events: {str(e)}")
            
            # Sleep briefly to avoid CPU hogging
            time.sleep(0.001) 