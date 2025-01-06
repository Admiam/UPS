import threading
from game import Game
from tkinter import messagebox
import tkinter as tk
import socket
import time


class ServerListener:
    def __init__(self, waiting_screen, client_socket, player_name, server, port):
        """
        Centralized server listener that manages communication and application state.

        Args:
            waiting_screen (tk.Tk): The main Tkinter application root window.
            client_socket (socket): The socket for server communication.
            player_name (str): The player's name.
        """
        self.waiting_screen = waiting_screen
        self.client_socket = client_socket
        self.player_name = player_name
        self.game_instance = None
        self.current_round = 0
        self.opponent_score = 0
        self.player_score = 0
        self.server = server
        self.port = port
        self.opponent_name = None
        self.waiting_screen_active = False
        self.ping_active = True
        self.is_reconnecting = False
        self.reconnection_attempts = 30  # Allow 30 seconds to reconnect
        self.last_pong_received = time.time()  # Timestamp for the last "pong"


        tk.Label(self.waiting_screen, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)

        # Cancel button to close the waiting screen and disconnect
        cancel_button = tk.Button(self.waiting_screen, text="Cancel", command=self.cancel_waiting)
        cancel_button.pack(pady=10)
        # Start the listener thread
        self.thread = threading.Thread(target=self.listen_to_server, daemon=True)
        self.thread.start()

        self.ping_thread = threading.Thread(target=self.ping, daemon=True)
        self.ping_thread.start()
        # Start ping thread
        # self.start_pinging()

        # Handle window close
        self.waiting_screen.protocol("WM_DELETE_WINDOW", self.cleanup)

    def start_pinging(self):
        """Start the ping process."""
        self.ping_thread = threading.Thread(target=self.ping, daemon=True)
        self.ping_thread.start()

    def ping(self):
        """Send periodic pings to the server."""
        # while self.ping_active:
        #     try:
        #         self.client_socket.sendall("RPS|ping".encode("utf-8"))
        #         threading.Event().wait(5)  # Wait for 5 seconds
        #         response = self.client_socket.recv(1024).decode("utf-8")
        #         if response.strip() != "RPS|pong;":
        #             print("Invalid pong response from server")
        #             self.disconnect_client()
        #             break
        #     except Exception as e:
        #         print(f"Ping error: {e}")
        #         self.disconnect_client()
        #         break
        while self.ping_active:
            try:
                if time.time() - self.last_pong_received > 3:  # Check for timeout
                    print("No pong received within timeout period.")
                    self.handle_internet_reconnection()
                    return

                self.client_socket.sendall(b"RPS|ping")
                print("Ping sent to server.")
                time.sleep(1)  # Send ping every 5 seconds

            except Exception as e:
                print(f"Ping error: {e}")
                self.handle_internet_reconnection()
                return

    def cleanup(self):
        """Handle cleanup when the window is closed."""
        self.ping_active = False
        self.client_socket.sendall(b"exit|" + self.player_name.encode("utf-8"))
        self.client_socket.close()
        self.waiting_screen.destroy()

    def register_game_instance(self, game_window):
        """Register the game instance for updates."""
        print("Game instance registered with ServerListener.")
        self.game_instance = game_window

    def listen_to_server(self):
        """Continuously listen for updates from the server."""
        buffer = ""
        while True:
            try:
                response = self.client_socket.recv(1024).decode("utf-8")
                print(f"Server response: {response}")
                buffer += response  # Append new data to the buffer

                while ";" in buffer:  # Process messages
                    message, buffer = buffer.split(";", 1)
                    if message.startswith("RPS|"):
                        print(f"Valid message: {message}")
                        if message == "RPS|pong":
                            self.last_pong_received = time.time()  # Update pong timestamp
                            print("Received valid pong from server.")
                            self.reset_reconnection_timer()  # Reset reconnection attempts

                        else:
                            print(f"Valid message: {message}")
                            self.route_server_message(message[4:])
                    else:
                        print(f"Invalid message received: {message}")
                        self.disconnect_client("Invalid message received")  # Disconnect with error
                        return  # Exit the thread after disconnecting

            except OSError as e:
                print(f"Connection lost or error listening to server: {e}")
                self.handle_internet_reconnection()
                break
            except Exception as e:
                print(f"Unexpected error: {e}")
                self.disconnect_client("Unexpected error occurred")
                break

    def reset_reconnection_timer(self):
        """Reset the reconnection attempts and continue normal operation."""
        if self.is_reconnecting:
            print("Client reconnected, resetting reconnection attempts.")
            self.is_reconnecting = False
            self.last_pong_received = time.time()  # Update pong timestamp
            self.ping_active = True  # Ensure pinging is active

    def route_server_message(self, message):
        """Route the server message to the appropriate handler."""
        if message == "connected":
            self.handle_connected()
        elif message.startswith("start"):
            self.handle_game_start("Group")
        elif "|" in message and message.startswith("Group"):
            self.handle_group_assignment(message)
        elif message.startswith("reconnect|success"):
            self.handle_reconnection(message)
        elif message.startswith("score|"):
            self.handle_score_update(message)
        elif message.startswith("result|"):
            self.handle_round_result(message)
        elif message.startswith("opponent_disconnected"):
            self.handle_opponent_disconnected()
        elif message.startswith("opponent_reconnected"):
            self.handle_opponent_reconnected()
        elif message.startswith("return_to_waiting"):
            self.handle_return_to_waiting()
        elif message == "error":
            self.handle_server_error()
        else:
            print(f"Unhandled message: {message}")

    def handle_connected(self):
        print("Connection confirmed by server. Waiting for group assignment...")
        self.show_waiting_screen()

    def handle_group_assignment(self, message):
        group_id, self.opponent_name = message.split("|", 1)
        print(f"Assigned to {group_id} with opponent {self.opponent_name}")
        self.update_gui_safe(self.start_game, group_id, 0, 0, 1)

    def handle_reconnection(self, message):
        parts = message.split("|")
        if len(parts) == 7:  # Format: reconnect|success|opponent_name|player_score|opponent_score|current_round|group_id
            self.opponent_name = parts[2]
            player_score = int(parts[3])
            opponent_score = int(parts[4])
            current_round = int(parts[5])
            group_id = parts[6]
            print(
                f"Reconnected to group {group_id}. Opponent: {self.opponent_name}, "
                f"Player Score: {player_score}, Opponent Score: {opponent_score}, Round: {current_round}"
            )
            self.update_gui_safe(self.start_game, group_id, player_score, opponent_score, current_round)

    def handle_score_update(self, message):
        parts = message.split("|")
        if len(parts) == 3:
            p1_score = int(parts[1])
            p2_score = int(parts[2])

            def update_scores():
                if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
                    self.game_instance.p1_score = p1_score
                    self.game_instance.p2_score = p2_score
                    print(
                        f"Updated scores - Player: {self.game_instance.p1_score}, Opponent: {self.game_instance.p2_score}"
                    )
                    self.game_instance.update_top_bar()
                    self.game_instance.start_new_round()  # Start the next round

                else:
                    print("Game window is not active.")

            self.update_gui_safe(update_scores)

    def handle_game_start(self, group_id, player_score=0, opponent_score=0, current_round=1):
        """Handle the start signal for the game."""
        print("Game can now start!")
        self.waiting_screen.withdraw()  # Hide the waiting screen

        if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
            print("Game instance already exists. Starting new round.")
            self.game_instance.game_active = True
            self.game_instance.start_new_round()
        else:
            print("Game instance does not exist or was destroyed. Creating a new game instance.")
            self.start_game(group_id, player_score, opponent_score, current_round)

    def handle_round_result(self, message):
        result = message.split("|")[1]
        print(f"Round result: {result}")

    def handle_opponent_disconnected(self):
        print("Opponent disconnected. Freezing game.")
        if self.game_instance:
            self.game_instance.freeze_game()

    def handle_opponent_reconnected(self):
        print("Opponent reconnected. Resuming game.")
        if self.game_instance:
            self.game_instance.unfreeze_game()
            self.game_instance.update_countdown(5)

    def handle_return_to_waiting(self):
        print("Returning to waiting screen.")
        # if self.game_instance:
            # self.game_instance.freeze_game()
            # self.update_gui_safe(self.return_to_waiting_screen)
        self.game_instance.game_window.destroy()
        if self.waiting_screen and self.waiting_screen.winfo_exists():
            self.waiting_screen.deiconify()

    def handle_server_error(self):
        messagebox.showerror("Error", "An error occurred on the server.")
        self.update_gui_safe(self.cancel_waiting)

    def start_game(self, group_name, player_score, opponent_score, current_round):
        """Create and transition to the game instance."""
        print("Starting game with opponent:", self.opponent_name)

        self.game_instance = Game(
            self,
            self.client_socket,
            group_name,
            self.player_name,
            self.opponent_name,
            self.waiting_screen,
            player_score,
            opponent_score,
            current_round,
        )

    def show_waiting_screen(self):
        """Show the waiting screen."""
        self.waiting_screen_active = True
        tk.Label(self.waiting_screen, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)

        # Cancel button to close the waiting screen and disconnect
        cancel_button = tk.Button(self.waiting_screen, text="Cancel", command=self.cancel_waiting)
        cancel_button.pack(pady=10)

    def cancel_waiting(self):
        """Cancel the waiting process and clean up."""
        if self.client_socket:
            self.client_socket.close()
        self.waiting_screen.destroy()

    def return_to_waiting_screen(self):
        """Return to the waiting screen."""
        if self.game_instance and self.game_instance.game_window:
            self.game_instance.game_window.destroy()
        self.show_waiting_screen()

    def update_gui_safe(self, func, *args):
        """Safely update the GUI from a different thread."""
        if self.waiting_screen and self.waiting_screen.winfo_exists():
            self.waiting_screen.after(0, func, *args)
        else:
            print("Root window no longer exists. Skipping GUI update.")

    def on_window_close(self):
        """Handle the window close event."""
        if self.client_socket:
            try:
                self.client_socket.close()  # Ensure the socket is closed
            except Exception as e:
                print(f"Error closing socket: {e}")
        self.waiting_screen.destroy()  # Destroy the Tkinter window
        print("Application closed.")
        exit(0)  # Exit the program

    def notify_server_lobby(self, player_name):
        """Notify the server that the player is returning to the lobby."""
        try:
            self.client_socket.sendall(f"return_to_lobby|{player_name}".encode("utf-8"))
        except Exception as e:
            print(f"Failed to notify server: {e}")

    def disconnect_client(self, error_message="Connection lost"):
        """Disconnect the client and reset state."""
        self.ping_active = False  # Stop pinging
        try:
            if self.client_socket:
                self.client_socket.close()  # Close the socket connection
                print("Disconnected from server.")
        except Exception as e:
            print(f"Error while closing client socket: {e}")

        # Show error message and transition to login on the main thread
        if self.waiting_screen and self.waiting_screen.winfo_exists():
            self.waiting_screen.after(0, self.show_error_and_login, error_message)
        else:
            print(f"Error displaying message: {error_message}")

    def show_error_and_login(self, error_message):
        """Show an error message and reopen the login window."""
        messagebox.showerror("Error", error_message)
        self.show_login_window()

    def show_login_window(self):
        """Show the login window after disconnecting."""
        # Destroy any existing windows
        if self.waiting_screen and self.waiting_screen.winfo_exists():
            self.waiting_screen.destroy()
        if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
            self.game_instance.game_window.destroy()

        # Create a new root for the login window
        self.waiting_screen.after(0, self.create_login_window)

    def create_login_window(self):
        """Create the login window."""
        new_root = tk.Tk()
        from login import Login  # Import Login dynamically to avoid circular imports
        Login(new_root)
        new_root.mainloop()

    def handle_internet_reconnection(self):
        """Handle reconnection attempts when the connection is lost."""
        if self.is_reconnecting:
            return  # Avoid overlapping reconnection attempts

        self.is_reconnecting = True
        self.ping_active = False  # Stop pinging
        self.game_instance.number_label.config(text=f"Reconnection")

        self.close_connection("Connection lost")  # Close the current connection
        # self.show_reconnecting_screen()  # Show reconnecting message

        # for attempt in range(self.reconnection_attempts):
        #     try:
        #         print(f"Reconnection attempt {attempt + 1}/{self.reconnection_attempts}...")
        #         self.client_socket.connect((self.server, self.port))  # Replace with server IP and port
        #         self.game_instance.number_lable.config(text=f"Reconnection attempt {attempt + 1}/{self.reconnection_attempts}...")
        #         print("Reconnected to server.")
        #         self.ping_active = True
        #         threading.Thread(target=self.ping, daemon=True).start()
        #         return
        #     except Exception as e:
        #         print(f"Reconnection failed: {e}")
        #         time.sleep(1)  # Wait 1 second before retrying

        for attempt in range(1, self.reconnection_attempts + 1):
            try:
                time.sleep(1)  # Wait before attempting to reconnect
                # self.client_socket.close()
                self.close_connection("Trying to connect")  # Close the current connection
                self.client_socket = socket.socket()
                self.client_socket.connect((self.server, self.port))  # Replace with your server address
                print("Reconnection successful.")
                self.reset_reconnection_timer()  # Reset reconnection attempts
                self.start_pinging()  # Resume pinging
                self.listen_to_server()  # Resume listening
                return
            except Exception as e:
                print(f"ERROR > Reconnection attempt {attempt}/{self.reconnection_attempts} failed: {e}")
                self.game_instance.number_label.config(text=f"{attempt}/{self.reconnection_attempts}")

        print("Failed to reconnect. Returning to login screen.")
        self.disconnect_client()

    def show_reconnecting_screen(self):
        """Display a reconnecting message to the user."""
        messagebox.showinfo("Reconnecting", "Attempting to reconnect... Please wait.")

    def close_connection(self, reason="Disconnected"):
        """Disconnect the client and clean up resources."""
        print(f"ERORR > {reason}")
        try:
            if self.client_socket:
                self.client_socket.close()  # Close the socket
                self.client_socket = None  # Clear the socket reference
        except Exception as e:
            print(f"Error closing socket: {e}")

        # Redirect to the login screen
        # self.update_gui_safe(self.show_login_window)
