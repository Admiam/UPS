import threading
from game import Game
from tkinter import messagebox
import tkinter as tk


class ServerListener:
    def __init__(self, waiting_screen, client_socket, player_name):
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
        self.opponent_name = None
        self.waiting_screen_active = False

        tk.Label(self.waiting_screen, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)

        # Cancel button to close the waiting screen and disconnect
        cancel_button = tk.Button(self.waiting_screen, text="Cancel", command=self.cancel_waiting)
        cancel_button.pack(pady=10)
        # Start the listener thread
        threading.Thread(target=self.listen_to_server, daemon=True).start()

    def register_game_instance(self, game_window):
        """Register the game instance for updates."""
        print("Game instance registered with ServerListener.")
        self.game_instance = game_window

    def listen_to_server(self):
        """Continuously listen for updates from the server."""
        while True:
            try:
                response = self.client_socket.recv(1024).decode("utf-8")
                print(f"Server response: {response}")
                self.route_server_message(response)
            except Exception as e:
                print(f"Connection lost or error listening to server: {e}")
                self.update_gui_safe(self.cancel_waiting)
                break

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

    def handle_return_to_waiting(self):
        print("Returning to waiting screen.")
        if self.game_instance:
            self.game_instance.freeze_game()
            self.update_gui_safe(self.return_to_waiting_screen)

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
