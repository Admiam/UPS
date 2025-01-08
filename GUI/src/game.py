import tkinter as tk
from PIL import Image, ImageTk
from tkinter import font as tkfont
import random
from functools import partial
import struct  # Make sure this is at the top of your file
import sys


class Game:
    def __init__(self, server, server_listener, group_name, player_name, opponent_name, root, p1_score, p2_score,
                 current_round):
        """
        Initialize the Game instance.

        Args:
            server_listener (ServerListener): The centralized server listener instance.
            group_name (str): The group name assigned by the server.
            player_name (str): The name of the player.
            opponent_name (str): The name of the opponent.
            root (tk.Tk): The waiting screen's root window.
            p1_score (int): Initial player score.
            p2_score (int): Initial opponent score.
            current_round (int): Current round number.
        """
        self.buttons = None
        self.button_frame = None
        self.image_label = None
        self.number_label = None
        self.big_font = None
        self.server_listener = server_listener  # Reference to the ServerListener
        self.server = server
        self.group_name = group_name
        self.player_name = player_name
        self.opponent_name = opponent_name.lstrip("\x00")
        self.root = root  # Reference to the waiting screen
        self.p1_score = p1_score
        self.p2_score = p2_score
        self.round = current_round
        self.max_rounds = 10
        self.selected_button = None
        self.is_frozen = False
        self.game_active = False  # Game starts inactive until the server signals
        self.ping_active = True  # Flag to control pinging

        # Create the game window
        self.game_window = tk.Toplevel()
        self.game_window.title(f"Game - {group_name}")

        # Initialize GUI components
        self.setup_top_bar()
        self.setup_mid_section()
        self.setup_bottom_section()

        # Notify the server of readiness
        self.send_ready_message()
        self.server.register_game_instance(self.game_window)  # Register with the ServerListener

        self.game_window.protocol("WM_DELETE_WINDOW", self.cleanup)

    def cleanup(self):
        """Clean up resources when the game window is closed."""
        self.ping_active = False  # Stop the pinging process
        try:
            self.server_listener.notify_server_lobby(self.player_name)  # Notify server
            self.server_listener.disconnect_client()  # Disconnect the socket
        except Exception as e:
            print(f"Cleanup error: {e}")
        finally:
            self.game_window.destroy()  # Destroy the game window
            print("Exiting program...")
            # Delay the exit to ensure threads have enough time to terminate gracefully
            self.root.after(100, sys.exit)

    def setup_top_bar(self):
        """Set up the top bar UI elements."""
        tk.Label(self.game_window, text=self.player_name).grid(row=0, column=0, padx=10)
        self.topbar_score1 = tk.Label(self.game_window, text=self.p1_score)
        self.topbar_score1.grid(row=0, column=1, padx=10)

        self.topbar_round = tk.Label(self.game_window, text=f"Round {self.round}", font=("Arial", 24))
        self.topbar_round.grid(row=0, column=2, padx=10)

        self.topbar_score2 = tk.Label(self.game_window, text=self.p2_score)
        self.topbar_score2.grid(row=0, column=3, padx=10)
        tk.Label(self.game_window, text=self.opponent_name).grid(row=0, column=4, padx=10)

    def setup_mid_section(self):
        """Set up the middle section of the game UI."""
        self.big_font = tkfont.Font(family="Helvetica", size=120, weight="bold")
        self.number_label = tk.Label(self.game_window, text="0", font=self.big_font)
        self.number_label.grid(row=1, column=2, pady=20)

    def setup_bottom_section(self):
        """Set up the bottom section with buttons."""
        self.button_frame = tk.Frame(self.game_window)
        self.button_frame.grid(row=2, column=2, pady=20)
        self.buttons = []

        image_files = ["../assets/malphite.png", "../assets/amumu.png", "../assets/gwen.png", "../assets/random.png"]
        for i, image_file in enumerate(image_files):
            image = Image.open(image_file).resize((100, 100), Image.LANCZOS)
            photo = ImageTk.PhotoImage(image)
            button = tk.Button(self.button_frame, image=photo, command=lambda idx=i: self.on_button_click(idx),
                               borderwidth=2, relief=tk.FLAT)
            button.image = photo
            button.pack(side=tk.LEFT, padx=10)
            self.buttons.append(button)

    # def send_ready_message(self):
    #     """Notify the server that the player is ready."""
    #     try:
    #         message = f"RPS|ready|{self.player_name}"
    #         self.server.send_to_server(message)
    #         print(f"Ready message sent: {message}")
    #     except Exception as e:
    #         print(f"Failed to send ready message: {e}")

    def update_top_bar(self):
        """Update the top bar with scores and round information."""
        if self.game_window and self.game_window.winfo_exists():
            self.topbar_score1.config(text=self.p1_score)
            self.topbar_score2.config(text=self.p2_score)
            self.topbar_round.config(text=f"Round {self.round}")
        else:
            print("Game window no longer exists. Skipping top bar update.")

    def start_new_round(self):
        """Prepare for a new round."""
        if not self.game_window.winfo_exists():
            print("Game window does not exist. Stopping new round setup.")
            return

        self.round += 1
        if self.round > self.max_rounds:
            print("Maximum rounds reached. Ending the game.")
            self.end_game()
            return

        self.selected_button = None
        self.reset_buttons()
        self.update_top_bar()

        if self.image_label:
            self.image_label.grid_forget()

        self.update_countdown(5)  # Start a 5-second countdown

    def update_countdown(self, count):
        """Update the countdown timer for the game."""
        if self.is_frozen:
            print("Countdown is frozen. Skipping update.")
            return  # Skip updates if the game is frozen

        if not self.game_window or not self.game_window.winfo_exists():
            print("Game window no longer exists. Stopping countdown.")
            return  # Stop countdown if the game window is not active

        if count > 0:
            print(f"Countdown: {count}")
            self.number_label.config(text=f"{count}")  # Update the countdown display
            self.game_window.after(1000, partial(self.update_countdown, count - 1))  # Continue countdown
        else:
            # Countdown finished, proceed with the round
            self.number_label.config(text="")  # Clear the countdown display
            # self.display_image()  # Show an image or other UI element for the round
            self.process_selection()  # Handle player's selection and send to the server
            # self.wait_for_reconnection_or_timeout()  # Check for reconnection or timeout

    # def display_image(self):
    #     """Show an image for the round."""
    #     try:
    #         # Load an image (replace with the path to your image)
    #         image = Image.open("../assets/round_start.png")  # Example image path
    #         image = image.resize((100, 100), Image.LANCZOS)  # Resize the image
    #         photo = ImageTk.PhotoImage(image)
    #
    #         if self.image_label is None:
    #             # Create the image label if it doesn't exist
    #             self.image_label = tk.Label(self.game_window, image=photo)
    #             self.image_label.image = photo  # Keep a reference to avoid garbage collection
    #             self.image_label.grid(row=1, column=2, pady=20)
    #         else:
    #             # Update the existing image label
    #             self.image_label.config(image=photo)
    #             self.image_label.image = photo
    #     except Exception as e:
    #         print(f"Error displaying image: {e}")

    # def wait_for_reconnection_or_timeout(self):
    #     """Wait for the opponent to reconnect or handle a timeout."""
    #     timeout = 10  # Set the timeout duration in seconds
    #     reconnect_event = threading.Event()  # Use an event to wait for reconnection
    #
    #     def listen_for_reconnection():
    #         """Listen for a reconnection signal from the server."""
    #         while not reconnect_event.is_set():
    #             try:
    #                 response = self.server_listener.recv(1024).decode('utf-8')  # Listen for server updates
    #                 print(f"Server response: {response}")
    #
    #                 if response.startswith("opponent_reconnected"):
    #                     reconnect_event.set()  # Stop waiting for reconnection
    #                     self.unfreeze_game()  # Resume the game
    #                     print("Opponent reconnected. Resuming game...")
    #                     return
    #
    #                 elif response.startswith("return_to_waiting"):
    #                     reconnect_event.set()
    #                     print("Opponent did not reconnect. Returning to waiting screen...")
    #                     self.freeze_game()
    #                     self.return_to_waiting_screen()
    #                     return
    #             except Exception as e:
    #                 print(f"Error while waiting for reconnection: {e}")
    #                 reconnect_event.set()
    #                 self.return_to_waiting_screen()  # Handle error by returning to the waiting screen
    #                 return
    #
    #     # Start listening for reconnection in a separate thread
    #     threading.Thread(target=listen_for_reconnection, daemon=True).start()
    #
    #     # Wait for reconnection or timeout
    #     if not reconnect_event.wait(timeout):  # Wait for the event or timeout
    #         print("Reconnection timed out. Returning to waiting screen.")
    #         self.freeze_game()
    #         self.return_to_waiting_screen()

    def reset_buttons(self):
        """Reset buttons to their default state."""
        for button in self.buttons:
            button.config(borderwidth=0, relief=tk.FLAT, highlightthickness=0)

    def freeze_game(self):
        """Freeze the game and disable buttons."""
        self.is_frozen = True
        for button in self.buttons:
            button.config(state=tk.DISABLED)

    def unfreeze_game(self, client_socket):
        """Unfreeze the game and enable buttons."""
        self.is_frozen = False
        self.round -= 1
        self.server_listener = client_socket
        self.start_new_round()
        for button in self.buttons:
            button.config(state=tk.NORMAL)
        # self.update_countdown(5)  # Start a 5-second countdown

    def return_to_waiting_screen(self):
        """Return to the waiting screen."""
        self.game_window.destroy()
        if self.root and self.root.winfo_exists():
            self.root.deiconify()
        self.notify_server_lobby(self.player_name)

    def process_selection(self):
        """Process the player's selection and notify the server."""
        choices = ["rock", "paper", "scissors"]
        if self.selected_button is not None:
            choice = choices[self.selected_button]
        else:
            choice = random.choice(choices)
            self.selected_button = choices.index(choice)

        self.send_choice_to_server(choice)
        print(f"Selection sent: {choice}")

    def send_ready_message(self):
        """Notify the server that the player is ready to start."""
        try:
            # Prepare components for the message
            magic = "RPS|".encode('utf-8')
            command = "ready|".encode('utf-8')
            player = self.player_name.encode('utf-8')

            # Calculate the total message length
            total_length = len(magic) + len(command) + len(player)

            # Pack the data
            buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(player)}s',
                                 magic, command, total_length, player)

            # Send the packed message to the server
            self.server_listener.sendall(buffer)
            print("Ready message sent to server:", buffer)
        except Exception as e:
            print(f"Failed to send ready message: {e}")

    def send_choice_to_server(self, choice):
        """Send the player's choice (as an index) to the server."""
        # choice_string = None
        # # print(f"Selected choice ID: {choice}")
        # choices = ["rock", "paper", "scissors"]
        # if isinstance(choice, int) and 0 <= choice < len(choices):
        #     choice_string = choices[choice]

        # print(f"Selected choice in string: {choice_string}")
        magic = "RPS|".encode('utf-8')
        command = "game|".encode('utf-8')
        message = choice.encode('utf-8')
        total_length = len(magic) + len(command) + len(choice)

        # Pack and send the data
        buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(message)}s',
                             magic, command, total_length, message)

        try:
            self.server_listener.sendall(buffer)
            print(f"Sent choice '{choice}' (as index) to server.")
        except Exception as e:
            print(f"Failed to send choice to server: {e}")

    def notify_server_lobby(self, player_name):
        """Notify the server that the player is returning to the lobby."""
        try:
            # Prepare the message components
            magic = "RPS|".encode('utf-8')
            command = "return_to_lobby|".encode('utf-8')
            player = player_name.encode('utf-8')

            # Calculate the total message length
            total_length = len(magic) + len(command) + len(player)

            # Pack the data
            buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(player)}s',
                                 magic, command, total_length, player)

            # Send the packed message to the server
            self.server_listener.sendall(buffer)
            print(f"Notified server: {buffer}")
        except Exception as e:
            print(f"Error notifying server to return to lobby: {e}")

    def on_button_click(self, button_index):
        # Deselect the previously selected button
        if self.selected_button is not None:
            self.buttons[self.selected_button].config(borderwidth=2, relief=tk.FLAT)

        # Handle random selection
        if button_index == 3:  # Random button
            button_index = random.randint(0, 2)

        # Select the new button
        self.selected_button = button_index
        self.buttons[button_index].config(borderwidth=2, relief=tk.SOLID, highlightbackground="white",
                                          highlightthickness=2)

    def end_game(self):
        """Ends the game and displays the final result."""
        # Calculate the winner
        winner_text = f"{self.player_name} Wins!" if self.p1_score > self.p2_score else f"{self.opponent_name} Wins!" if self.p2_score > self.p1_score else "It's a Tie!"

        final_text = f"{winner_text}\nFinal Score: {self.player_name} {self.p1_score} : {self.p2_score} {self.opponent_name}"

        # Clear the existing labels and buttons
        for widget in self.game_window.winfo_children():
            widget.grid_forget()  # Hide all widgets

        # Create a label for the final result
        result_label = tk.Label(self.game_window, text=final_text, font=("Arial", 24))
        result_label.pack(pady=20)

        # Create a button to return to the main lobby
        return_button = tk.Button(self.game_window, text="Return to Lobby", command=self.return_to_waiting_screen)
        return_button.pack(pady=10)