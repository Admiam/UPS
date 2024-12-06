# class Game:
#     def __init__(self, server_listener, group_name, player_name, opponent_name, waiting_screen_root, p1_score, p2_score,
#                  current_round):
#         """
#         Initialize the Game instance.
#
#         Args:
#             server_listener (ServerListener): The centralized server listener instance.
#             group_name (str): The group name assigned by the server.
#             player_name (str): The name of the player.
#             opponent_name (str): The name of the opponent.
#             waiting_screen_root (tk.Tk): The waiting screen's root window.
#             p1_score (int): Initial player score.
#             p2_score (int): Initial opponent score.
#             current_round (int): Current round number.
#         """
#         self.server_listener = server_listener  # Reference to the ServerListener
#         self.group_name = group_name
#         self.player_name = player_name
#         self.opponent_name = opponent_name.lstrip("\x00")
#         self.waiting_screen_root = waiting_screen_root  # Reference to the waiting screen
#         self.p1_score = p1_score
#         self.p2_score = p2_score
#         self.round = current_round
#         self.max_rounds = 10
#         self.selected_button = None
#         self.is_frozen = False
#         self.game_active = False  # Game starts inactive until the server signals
#
#         # Create the game window
#         self.game_window = tk.Toplevel()
#         self.game_window.title(f"Game - {group_name}")
#
#         # Initialize GUI components
#         self.setup_top_bar()
#         self.setup_mid_section()
#         self.setup_bottom_section()
#
#         # Notify the server of readiness
#         self.send_ready_message()
#         self.server_listener.register_game_instance(self)  # Register with the ServerListener
#
#     def setup_top_bar(self):
#         """Set up the top bar UI elements."""
#         tk.Label(self.game_window, text=self.player_name).grid(row=0, column=0, padx=10)
#         self.topbar_score1 = tk.Label(self.game_window, text=self.p1_score)
#         self.topbar_score1.grid(row=0, column=1, padx=10)
#
#         self.topbar_round = tk.Label(self.game_window, text=f"Round {self.round}", font=("Arial", 24))
#         self.topbar_round.grid(row=0, column=2, padx=10)
#
#         self.topbar_score2 = tk.Label(self.game_window, text=self.p2_score)
#         self.topbar_score2.grid(row=0, column=3, padx=10)
#         tk.Label(self.game_window, text=self.opponent_name).grid(row=0, column=4, padx=10)
#
#     def setup_mid_section(self):
#         """Set up the middle section of the game UI."""
#         self.big_font = tkfont.Font(family="Helvetica", size=120, weight="bold")
#         self.number_label = tk.Label(self.game_window, text="0", font=self.big_font)
#         self.number_label.grid(row=1, column=2, pady=20)
#
#     def setup_bottom_section(self):
#         """Set up the bottom section with buttons."""
#         self.button_frame = tk.Frame(self.game_window)
#         self.button_frame.grid(row=2, column=2, pady=20)
#         self.buttons = []
#
#         image_files = ["../assets/malphite.png", "../assets/amumu.png", "../assets/gwen.png", "../assets/random.png"]
#         for i, image_file in enumerate(image_files):
#             image = Image.open(image_file).resize((100, 100), Image.LANCZOS)
#             photo = ImageTk.PhotoImage(image)
#             button = tk.Button(self.button_frame, image=photo, command=lambda idx=i: self.on_button_click(idx),
#                                borderwidth=2, relief=tk.FLAT)
#             button.image = photo
#             button.pack(side=tk.LEFT, padx=10)
#             self.buttons.append(button)
#
#     def send_ready_message(self):
#         """Notify the server that the player is ready."""
#         try:
#             message = f"RPS|ready|{self.player_name}"
#             self.server_listener.send_to_server(message)
#             print(f"Ready message sent: {message}")
#         except Exception as e:
#             print(f"Failed to send ready message: {e}")
#
#     def update_top_bar(self):
#         """Update the top bar with scores and round information."""
#         if self.game_window and self.game_window.winfo_exists():
#             self.topbar_score1.config(text=self.p1_score)
#             self.topbar_score2.config(text=self.p2_score)
#             self.topbar_round.config(text=f"Round {self.round}")
#         else:
#             print("Game window no longer exists. Skipping top bar update.")
#
#     def start_new_round(self):
#         """Prepare for a new round."""
#         if not self.game_window.winfo_exists():
#             print("Game window does not exist. Stopping new round setup.")
#             return
#
#         self.selected_button = None
#         self.reset_buttons()
#         self.update_top_bar()
#
#         if self.image_label:
#             self.image_label.grid_forget()
#
#         self.update_countdown(5)  # Start a 5-second countdown
#
#     def reset_buttons(self):
#         """Reset buttons to their default state."""
#         for button in self.buttons:
#             button.config(borderwidth=0, relief=tk.FLAT, highlightthickness=0)
#
#     def freeze_game(self):
#         """Freeze the game and disable buttons."""
#         self.is_frozen = True
#         for button in self.buttons:
#             button.config(state=tk.DISABLED)
#
#     def unfreeze_game(self):
#         """Unfreeze the game and enable buttons."""
#         self.is_frozen = False
#         for button in self.buttons:
#             button.config(state=tk.NORMAL)
#
#     def return_to_waiting_screen(self):
#         """Return to the waiting screen."""
#         self.game_window.destroy()
#         if self.waiting_screen_root and self.waiting_screen_root.winfo_exists():
#             self.waiting_screen_root.deiconify()
#         self.server_listener.notify_server_lobby(self.player_name)
#
#     def process_selection(self):
#         """Process the player's selection and notify the server."""
#         choices = ["Rock", "Paper", "Scissors"]
#         if self.selected_button is not None:
#             choice = choices[self.selected_button]
#         else:
#             choice = random.choice(choices)
#             self.selected_button = choices.index(choice)
#
#         self.server_listener.send_to_server(f"RPS|game|{choice}")
#         print(f"Selection sent: {choice}")
