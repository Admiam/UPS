# import tkinter as tk
# from PIL import Image, ImageTk
# from tkinter import font as tkfont
# import random
# import struct  # Make sure this is at the top of your file
# from functools import partial
# import threading
#
#
# class Game:
#     def __init__(self, lobby_app, group_name, player_name, opponent_name, waiting_screen_root, p1_score, p2_score,
#                  current_round):
#         self.buttons = None
#         self.selected_button = None
#         self.topbar_score1 = None
#         self.button_frame = None
#         self.big_font = None
#         self.number_label = None
#         self.lobby_app = lobby_app  # Store reference to lobby_app
#         self.group_name = group_name
#         self.opponent_name = opponent_name.lstrip("\x00")
#         self.player_name = player_name
#         self.waiting_screen_root = waiting_screen_root  # Reference to the waiting screen
#         self.round = current_round
#         self.max_rounds = 10
#         self.p1_score = p1_score
#         self.p2_score = p2_score
#         self.image_label = None  # Initialize image_label to None
#         self.game_active = False  # Game starts inactive until server signals
#         self.is_frozen = False
#
#         # Create the game window
#         self.game_window = tk.Toplevel()
#         self.game_window.title(f"Game - {group_name}")
#
#         # Set up UI elements
#         self.setup_top_bar()
#         self.setup_mid_section()
#         self.setup_bottom_section()
#
#         if not self.game_active:  # Only send the ready message if the game is not active
#             self.send_ready_message()
#             self.wait_for_start_signal()  # Start listening for updates in the background
#
#     def wait_for_start_signal(self):
#         """Wait for the server to send the 'start' message to activate the game."""
#
#         def wait_for_ready():
#             while not self.game_active:
#                 try:
#                     response = self.lobby_app.recv(1024).decode('utf-8')
#                     print(f"Server response while waiting: {response}")
#
#                     if response == "start":
#                         print("Game can now start!")
#                         self.game_active = True
#
#                         # Check if the game window exists before starting a new round
#                         if self.game_window and self.game_window.winfo_exists():
#                             self.start_new_round()  # Start the game after receiving the "start" signal
#                         else:
#                             print("Game window no longer exists. Cannot start a new round.")
#                         break
#
#                 except Exception as e:
#                     print(f"Error while waiting for start signal: {e}")
#                     self.return_to_waiting_screen()  # Return to the waiting screen on error
#                     break
#
#         import threading
#         threading.Thread(target=wait_for_ready, daemon=True).start()
#
#     def setup_top_bar(self):
#         topbar_p1 = tk.Label(self.game_window, text=self.player_name)
#         topbar_p1.grid(row=0, column=0, padx=10)
#         self.topbar_score1 = tk.Label(self.game_window, text=self.p1_score)
#         self.topbar_score1.grid(row=0, column=1, padx=10)
#
#         # Round
#         self.topbar_round = tk.Label(self.game_window, text=f"Round {self.round}", font=("Arial", 24))
#         self.topbar_round.grid(row=0, column=2, padx=10)
#
#         # Player 2
#         self.topbar_score2 = tk.Label(self.game_window, text=self.p2_score)
#         self.topbar_score2.grid(row=0, column=3, padx=10)
#
#         topbar_p2 = tk.Label(self.game_window, text=self.opponent_name)
#         topbar_p2.grid(row=0, column=4, padx=10)
#
#     def setup_mid_section(self):
#         self.big_font = tkfont.Font(family="Helvetica", size=120, weight="bold")
#         self.number_label = tk.Label(self.game_window, text="0", font=self.big_font)
#         self.number_label.grid(row=1, column=2, pady=20)
#
#     def on_button_click(self, button_index):
#         # Deselect the previously selected button
#         if self.selected_button is not None:
#             self.buttons[self.selected_button].config(borderwidth=2, relief=tk.FLAT)
#
#         # Handle random selection
#         if button_index == 3:  # Random button
#             button_index = random.randint(0, 2)
#
#         # Select the new button
#         self.selected_button = button_index
#         self.buttons[button_index].config(borderwidth=2, relief=tk.SOLID, highlightbackground="white",
#                                           highlightthickness=2)
#
#     def display_image(self):
#         image2 = Image.open("../assets/malphite.png")
#         image2 = image2.resize((100, 100), Image.LANCZOS)
#         photo2 = ImageTk.PhotoImage(image2)
#         if self.image_label is None:
#             self.image_label = tk.Label(self.game_window, image=photo2)
#             self.image_label.grid(row=1, column=2, pady=20)
#         else:
#             self.image_label.config(image=photo2)
#         self.image_label.photo = photo2  # Keep a reference to avoid garbage collection
#
#     def setup_bottom_section(self):
#         self.button_frame = tk.Frame(self.game_window)
#         self.button_frame.grid(row=2, column=2, pady=20)
#         self.buttons = []
#
#         image_files = ["../assets/malphite.png", "../assets/amumu.png", "../assets/gwen.png", "../assets/random.png"]
#
#         for i, image_file in enumerate(image_files):
#             image = Image.open(image_file)
#             image = image.resize((100, 100), Image.LANCZOS)
#             photo = ImageTk.PhotoImage(image)
#
#             button = tk.Button(self.button_frame, image=photo, command=lambda idx=i: self.on_button_click(idx),
#                                borderwidth=2, relief=tk.FLAT)
#             button.image = photo
#             button.pack(side=tk.LEFT, padx=10)
#             self.buttons.append(button)
#
#     def send_choice_to_server(self, choice):
#         """Send the player's choice (as an index) to the server."""
#         choice_string = None
#         # print(f"Selected choice ID: {choice}")
#         choices = ["rock", "paper", "scissors"]
#         if isinstance(choice, int) and 0 <= choice < len(choices):
#             choice_string = choices[choice]
#
#         # print(f"Selected choice in string: {choice_string}")
#         magic = "RPS|".encode('utf-8')
#         command = "game|".encode('utf-8')
#         message = choice_string.encode('utf-8')
#         total_length = len(magic) + len(command) + len(choice_string)
#
#         # Pack and send the data
#         buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(message)}s',
#                              magic, command, total_length, message)
#
#         try:
#             self.lobby_app.sendall(buffer)
#             print(f"Sent choice '{choice_string}' (as index) to server.")
#         except Exception as e:
#             print(f"Failed to send choice to server: {e}")
#
#     def start_new_round(self):
#         """Resets the countdown and buttons for a new round."""
#         if not self.game_window.winfo_exists():  # Ensure the game window exists
#             print("Game window no longer exists. Stopping new round.")
#             return
#         print("test1")
#         self.selected_button = None  # Clear the previous selection
#         self.reset_buttons()  # Reset the button visuals
#         self.update_top_bar()  # Update round number and score
#
#         print("test5")
#
#         if self.image_label is not None:
#             self.image_label.grid_forget()
#
#         print("test5")
#
#         self.update_countdown(5)  # Start a 9-second countdown
#
#     def reset_buttons(self):
#         """Resets all buttons to their default appearance."""
#         if not hasattr(self, "buttons") or not self.buttons:
#             print("Buttons list is empty or undefined. Skipping reset.")
#             return
#         print("test2")
#
#         for button in self.buttons:
#             button.config(borderwidth=0, relief=tk.FLAT, highlightthickness=0)
#             print("test3")
#
#     def update_countdown(self, count):
#         if self.is_frozen:
#             print("test6")
#             return  # Skip countdown updates while the game is frozen
#
#         if not self.game_window.winfo_exists():  # Ensure the game window exists
#             print("Game window no longer exists. Stopping countdown.")
#             return
#
#         if count > 0:
#             print(f"Countdown: {count}")  # Debug statement to track countdown
#             print("test7")
#
#             self.number_label.config(text=f"{count}")
#             print("test8")
#
#             self.game_window.after(1000, partial(self.update_countdown, count - 1))
#
#             print("test9")
#
#         else:
#             # Once countdown finishes, hide the number label and display image
#             print("test10")
#
#             self.number_label.config(text="")
#             print("test11")
#
#             self.display_image()
#             print("test12")
#             self.process_selection()  # Process selection and send to server
#             self.wait_for_reconnection_or_timeout()  # Handle reconnection or timeout
#             print("test13")
#
#             # Move to the next round or end the game
#             self.round += 1
#             if self.round <= self.max_rounds:
#                 self.start_new_round()
#                 print("test14")
#
#             else:
#                 self.end_game()
#
#     def wait_for_reconnection_or_timeout(self):
#         """Wait for the opponent to reconnect or handle a timeout."""
#         timeout = 10  # Adjust the timeout as needed
#         reconnect_event = threading.Event()
#
#         def listen_for_reconnection():
#             while not reconnect_event.is_set():
#                 try:
#                     response = self.lobby_app.recv(1024).decode('utf-8')
#                     print(f"Server response: {response}")
#
#                     if response.startswith("opponent_reconnected"):
#                         reconnect_event.set()  # Stop waiting for reconnection
#                         self.unfreeze_game()  # Unfreeze the game and resume play
#                         print("Opponent reconnected. Resuming game...")
#                         return
#
#                     elif response.startswith("return_to_waiting"):
#                         reconnect_event.set()
#                         print("Opponent did not reconnect. Returning to waiting screen...")
#                         self.freeze_game()  # Freeze game before returning
#                         self.waiting_screen_root.update_gui_safe(self.return_to_waiting_screen)
#                         return
#
#                 except Exception as e:
#                     print(f"Error while waiting for reconnection: {e}")
#                     reconnect_event.set()
#                     self.return_to_waiting_screen()  # Handle errors by returning to the waiting screen
#                     return
#
#         # Start listening for reconnection in a separate thread
#         threading.Thread(target=listen_for_reconnection, daemon=True).start()
#
#         # Wait for the timeout or reconnection
#         if not reconnect_event.wait(timeout):  # Wait for the event or timeout
#             print("Reconnection timed out. Returning to waiting screen.")
#             self.freeze_game()
#             self.return_to_waiting_screen()
#
#     def process_selection(self):
#         """Process the player's selection and send it to the server."""
#         choices = ["Rock", "Paper", "Scissors"]
#         if self.selected_button is not None:
#             player_choice = choices[self.selected_button]
#             print(f"Selected option: {player_choice}")
#
#             # Send choice to server
#             self.send_choice_to_server(self.selected_button)
#             return
#         else:
#             print("No selection made, selecting random.")
#             random_index = random.randint(0, 2)
#             random_choice = choices[random_index]
#             self.send_choice_to_server(random_index)  # Send the choice string
#             return
#
#     def update_top_bar(self):
#         """Updates the score and round number in the top bar."""
#         if not self.game_window.winfo_exists():  # Check if the window exists
#             print("Game window does not exist. Skipping update.")
#             return
#         print("test4")
#
#         try:
#             self.topbar_score1.config(text=self.p1_score)
#             self.topbar_score2.config(text=self.p2_score)
#             self.topbar_round.config(text=f"Round {self.round}")
#             print("test5")
#
#         except tk.TclError as e:
#             print(f"Error updating top bar: {e}")
#
#     def end_game(self):
#         """Ends the game and displays the final result."""
#         # Calculate the winner
#         winner_text = f"{self.player_name} Wins!" if self.p1_score > self.p2_score else f"{self.opponent_name} Wins!" if self.p2_score > self.p1_score else "It's a Tie!"
#
#         final_text = f"{winner_text}\nFinal Score: {self.player_name} {self.p1_score} : {self.p2_score} {self.opponent_name}"
#
#         # Clear the existing labels and buttons
#         for widget in self.game_window.winfo_children():
#             widget.grid_forget()  # Hide all widgets
#
#         # Create a label for the final result
#         result_label = tk.Label(self.game_window, text=final_text, font=("Arial", 24))
#         result_label.pack(pady=20)
#
#         # Create a button to return to the main lobby
#         return_button = tk.Button(self.game_window, text="Return to Lobby", command=self.return_to_waiting_screen)
#         return_button.pack(pady=10)
#
#     def return_to_waiting_screen(self):
#         """Return to the waiting screen and notify the server."""
#         if self.game_window and self.game_window.winfo_exists():
#             self.game_window.withdraw()  # Hide the game window
#         else:
#             print("Game window no longer exists.")
#
#         if self.waiting_screen_root and self.waiting_screen_root.winfo_exists():
#             self.waiting_screen_root.deiconify()  # Show the waiting screen
#         else:
#             print("Waiting screen root no longer exists.")
#
#         try:
#             magic = "RPS|".encode('utf-8')
#             command = "lobby|".encode('utf-8')
#             message = self.player_name.encode('utf-8')
#             total_length = len(magic) + len(command) + len(message)
#
#             # Pack and send the data
#             buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(message)}s',
#                                  magic, command, total_length, message)
#             self.lobby_app.send(buffer)
#             print(f"Notified server: {buffer}")
#         except Exception as e:
#             print(f"Error notifying server to re-enter queue: {e}")
#
#     # def listen_for_server_updates(self):
#     #     """Continuously listen for updates from the server."""
#     #     def handle_server_responses():
#     #         while True:
#     #             try:
#     #                 response = self.lobby_app.recv(1024).decode('utf-8')
#     #                 print(f"Server response: {response}")
#     #                 self.process_server_message(response)  # Process the received message
#     #             except Exception as e:
#     #                 print(f"Error listening to server: {e}")
#     #                 break
#     #
#     #     import threading
#     #     threading.Thread(target=handle_server_responses, daemon=True).start()
#
#     # def process_server_message(self, message):
#     #     """Handle updates like opponent disconnection or reconnection."""
#     #     if message.startswith("start"):
#     #         print("Game can now start!")
#     #         self.game_active = True
#     #         self.start_new_round()
#     #     elif message.startswith("score|"):
#     #         parts = message.split("|")
#     #         if len(parts) == 3:
#     #             self.p1_score = int(parts[1])
#     #             self.p2_score = int(parts[2])
#     #             print(f"Updated scores - Player: {self.p1_score}, Opponent: {self.p2_score}")
#     #             self.update_top_bar()  # Update the UI with new scores
#     #     elif message.startswith("result|"):
#     #         result = message.split("|")[1]
#     #         print(f"Round result: {result}")
#     #     elif message.startswith("opponent_disconnected"):
#     #         self.freeze_game()
#     #     elif message.startswith("opponent_reconnected"):
#     #         self.unfreeze_game()
#     #         print("Opponent reconnected. Resuming game...")
#     #     elif message.startswith("return_to_waiting"):
#     #         self.freeze_game()
#     #         self.return_to_waiting_screen()
#
#     # def wait_for_server_response(self):
#     #     """Wait for server response to update scores and handle round results."""
#     #     try:
#     #         response = self.lobby_app.recv(1024).decode('utf-8')
#     #         print(f"Server response: {response}")
#     #         self.process_server_message(response)
#     #     except Exception as e:
#     #         print(f"Error receiving server response: {e}")
#
#     def freeze_game(self):
#         """Freeze the game and show a message to the player."""
#         self.is_frozen = True
#         # self.number_label.config(text=message)
#         for button in self.buttons:
#             button.config(state=tk.DISABLED)
#
#     def unfreeze_game(self):
#         """Unfreeze the game and allow the player to continue."""
#         self.is_frozen = False
#         # self.number_label.config(text="")
#         for button in self.buttons:
#             button.config(state=tk.NORMAL)
#
#     def send_ready_message(self):
#         """Notify the server that the player is ready to start."""
#         try:
#             # Prepare components for the message
#             magic = "RPS|".encode('utf-8')
#             command = "ready|".encode('utf-8')
#             player = self.player_name.encode('utf-8')
#
#             # Calculate the total message length
#             total_length = len(magic) + len(command) + len(player)
#
#             # Pack the data
#             buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(player)}s',
#                                  magic, command, total_length, player)
#
#             # Send the packed message to the server
#             self.lobby_app.sendall(buffer)
#             print("Ready message sent to server:", buffer)
#         except Exception as e:
#             print(f"Failed to send ready message: {e}")
