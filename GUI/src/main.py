# main.py
import tkinter as tk
from login import Login

def on_close():
    """Handle the main window close event."""
    print("Application is closing.")
    root.destroy()
    exit(0)  # Ensure the program exits

# Initialize the main window
root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", on_close)  # Bind the close event
login_app = Login(root)
root.mainloop()
