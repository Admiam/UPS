# Login.py
import tkinter as tk
from tkinter import messagebox
from connect import connect_to_server  # Import the connect function
from serverListener import ServerListener  # Import the centralized ServerListener


class Login:
    def __init__(self, root):
        self.root = root
        self.root.title("Login")

        # Player Name label and entry
        tk.Label(root, text="Player Name:").grid(row=1, column=0, padx=10, pady=10)
        self.entry_player = tk.Entry(root)
        self.entry_player.grid(row=1, column=1, padx=10, pady=10)

        tk.Label(root, text="IP:").grid(row=2, column=0, padx=10, pady=10)
        self.ip = tk.Entry(root)
        self.ip.grid(row=2, column=1, padx=10, pady=10)

        # Login button
        self.login_button = tk.Button(root, text="Login", command=self.attempt_login)
        self.login_button.grid(row=3, column=0, columnspan=2, pady=10)

    def attempt_login(self):
        # Hard-coded server IP (or you can re-enable server IP entry in the GUI)
        server_ip = self.ip.get()
        player_name = self.entry_player.get()

        if not player_name:
            messagebox.showerror("Error", "Player Name is required.")
            return

        # Attempt to connect using connect.py
        client_socket = connect_to_server(server_ip, player_name)

        if client_socket:
            messagebox.showinfo("Success", "Connected to the server!")
            self.root.withdraw()  # Hide login window

            # Open the waiting screen via ServerListener
            waiting_screen = tk.Toplevel(self.root)  # Create the waiting screen window
            waiting_screen.title("Waiting for Players")

            ServerListener(waiting_screen, client_socket, player_name)  # Centralized listener and screen manager
        else:
            messagebox.showerror("Connection Failed", "Could not connect to the server.")
