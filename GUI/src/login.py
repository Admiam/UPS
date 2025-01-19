# Login.py
import tkinter as tk
from tkinter import messagebox
from connect import connect_to_server  # Import the connect function
from serverListener import ServerListener  # Import the centralized ServerListener


class Login:
    def __init__(self, root):
        self.root = root
        self.root.title("Login")
        self.server_ip = "127.0.0.1"
        self.port = 4242

        # Player Name label and entry
        tk.Label(root, text="Player Name:").grid(row=1, column=0, padx=10, pady=10)
        self.entry_player = tk.Entry(root)
        self.entry_player.grid(row=1, column=1, padx=10, pady=10)

        tk.Label(root, text="IP:").grid(row=2, column=0, padx=10, pady=10)
        self.ip = tk.Entry(root)
        self.ip.grid(row=2, column=1, padx=10, pady=10)

        tk.Label(root, text="Port:").grid(row=3, column=0, padx=10, pady=10)
        self.port_input = tk.Entry(root)
        self.port_input.grid(row=3, column=1, padx=10, pady=10)

        # Login button
        self.login_button = tk.Button(root, text="Login", command=self.attempt_login)
        self.login_button.grid(row=4, column=0, columnspan=2, pady=10)

    def attempt_login(self):
        # Hard-coded server IP (or you can re-enable server IP entry in the GUI)
        self.server_ip = self.ip.get()
        port_str = self.port_input.get()
        try:
            # # Check if `self.port` is an Entry widget or already an int
            # if isinstance(self.port, tk.Entry):
            #     port_str = self.port_input.get()  # Get the value from the Entry widget
            # elif isinstance(self.port, int):
            #     port_str = str(self.port_input)  # Convert the int to a string for processing
            #     port_str = self.port_input.get()  # Get the value from the Entry widget
            #
            # else:
            #     messagebox.showerror("Error", "Port input is invalid.")
            #     return

            # Get the port value from the input field
            # port_str = self.port.get()

            # Verify it is a number
            self.port = int(port_str)

            # Check if the port is in the valid range (1–65535 for TCP/UDP ports)
            if not (1 <= self.port <= 65535):
                raise ValueError(f"Invalid port range: {self.port}")

            print(f"Valid port entered: {self.port}")
        except ValueError as e:
            # Handle invalid input (e.g., not a number or out of range)
            print(f"Error: {e}")
            messagebox.showerror("Invalid Input", "Please enter a valid port number (1–65535).")
            return
            # self.port = 4242  # Reset the port value

        player_name = self.entry_player.get()

        if not player_name:
            messagebox.showerror("Error", "Player Name is required.")
            return

        # Attempt to connect using connect.py
        client_socket = connect_to_server(self.server_ip, player_name, self.port)

        if client_socket:
            messagebox.showinfo("Success", "Connected to the server!")
            self.root.withdraw()  # Hide login window

            # Open the waiting screen via ServerListener
            waiting_screen = tk.Toplevel(self.root)  # Create the waiting screen window
            waiting_screen.title("Waiting for Players")

            ServerListener(waiting_screen, client_socket, player_name, self.server_ip, self.port)  # Centralized listener and screen manager
        else:
            messagebox.showerror("Connection Failed", "Could not connect to the server.")
