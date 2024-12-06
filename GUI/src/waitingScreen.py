# # WaitingScreen.py
# import tkinter as tk
# from tkinter import messagebox
# import threading
# from game import Game
#
#
# class WaitingScreen:
#     def __init__(self, root, client_socket, player_name):
#         self.game_instance = None
#         self.current_round = None
#         self.opponent_score = None
#         self.player_score = None
#         self.root = root
#         self.client_socket = client_socket
#         self.player_name = player_name
#         self.root.title("Waiting for Group Assignment")
#         self.opponent_name = None
#
#         tk.Label(root, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)
#
#         # Cancel button to close the waiting screen and disconnect
#         cancel_button = tk.Button(root, text="Cancel", command=self.cancel_waiting)
#         cancel_button.pack(pady=10)
#
#         # Start a thread to listen for the server's response
#         threading.Thread(target=self.listen_to_server, daemon=True).start()
#
#     def listen_to_server(self):
#         """Continuously listen for updates from the server."""
#         while True:
#             try:
#                 response = self.client_socket.recv(1024).decode('utf-8')
#                 print(f"Server response: {response}")
#                 self.route_server_message(response)  # Route the message to the appropriate handler
#             except Exception as e:
#                 print(f"Connection lost or error listening to server: {e}")
#                 self.update_gui_safe(self.cancel_waiting)
#                 break
#
#     def route_server_message(self, message):
#         """Route the server message to the appropriate handler."""
#         print("Message received:", message)
#         if message == "connected":
#             self.handle_connected()
#         elif message.startswith("start"):
#             self.handle_game_start("Group")
#         elif "|" in message and message.startswith("Group"):
#             self.handle_group_assignment(message)
#         elif message.startswith("reconnect|success"):
#             self.handle_reconnection(message)
#         elif message.startswith("score|"):
#             self.handle_score_update(message)
#         elif message.startswith("result"):
#             self.handle_round_result(message)
#         elif message.startswith("opponent_disconnected"):
#             self.handle_opponent_disconnected()
#         elif message.startswith("opponent_reconnected"):
#             self.handle_opponent_reconnected()
#         elif message.startswith("return_to_waiting"):
#             self.handle_return_to_waiting()
#         elif message == "error":
#             self.handle_server_error()
#         else:
#             print(f"Unhandled message: {message}")
#
#     def handle_connected(self):
#         print("Connection confirmed by server. Waiting for group assignment...")
#
#     def handle_group_assignment(self, message):
#         group_id, self.opponent_name = message.split("|", 1)
#         print(f"Assigned to {group_id} with opponent {self.opponent_name}")
#         self.start_game(group_id, 0, 0, 1)
#
#     def handle_reconnection(self, message):
#         parts = message.split("|")
#         if len(parts) == 7:  # Format: reconnect|success|opponent_name|player_score|opponent_score|current_round|group_id
#             self.opponent_name = parts[2]
#             player_score = int(parts[3])
#             opponent_score = int(parts[4])
#             current_round = int(parts[5])
#             group_id = parts[6]
#             print(
#                 f"Reconnected to group {group_id}. Opponent: {self.opponent_name}, Player Score: {player_score}, Opponent Score: {opponent_score}, Round: {current_round}")
#             self.start_game(group_id, player_score, opponent_score, current_round)
#
#     def handle_score_update(self, message):
#         parts = message.split("|")
#         if len(parts) == 3:
#             p1_score = int(parts[1])
#             p2_score = int(parts[2])
#
#             def update_scores():
#                 if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
#                     self.game_instance.p1_score = p1_score
#                     self.game_instance.p2_score = p2_score
#                     print(
#                         f"Updated scores - Player: {self.game_instance.p1_score}, Opponent: {self.game_instance.p2_score}")
#                     self.game_instance.topbar_score1.config(text=self.game_instance.p1_score)
#                     self.game_instance.topbar_score2.config(text=self.game_instance.p2_score)
#                     self.game_instance.topbar_round.config(text=f"Round {self.game_instance.round}")
#                 else:
#                     print("Game window is not active. Attempting to reactivate.")
#                     self.reactivate_game_window()
#
#             # Schedule the update on the main thread
#             if self.root and self.root.winfo_exists():
#                 self.root.after(0, update_scores)
#
#     def reactivate_game_window(self):
#         """Reactivates the game window if it's inactive."""
#         if self.game_instance and self.game_instance.game_window:
#             if not self.game_instance.game_window.winfo_exists():
#                 print("Game window does not exist. Cannot reactivate.")
#                 return
#
#             try:
#                 self.game_instance.game_window.deiconify()  # Reactivate the window
#                 print("Reactivated the game window.")
#             except tk.TclError as e:
#                 print(f"Error reactivating game window: {e}")
#         else:
#             print("Game instance or window not available for reactivation.")
#
#     # def handle_score_update(self, message):
#     #     parts = message.split("|")
#     #     if len(parts) == 3:
#     #         if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
#     #             self.game_instance.p1_score = int(parts[1])
#     #             self.game_instance.p2_score = int(parts[2])
#     #             print(f"Updated scores - Player: {self.game_instance.p1_score}, Opponent: {self.game_instance.p2_score}")
#     #             self.game_instance.topbar_score1.config(text=self.game_instance.p1_score)
#     #             self.game_instance.topbar_score2.config(text=self.game_instance.p2_score)
#     #             self.game_instance.topbar_round.config(text=f"Round {self.game_instance.round}")
#     #         else:
#     #             print("Game window or widgets no longer exist.")
#     def handle_server_error(self):
#         messagebox.showerror("Error", "An error occurred on the server.")
#         self.cancel_waiting()
#
#     def handle_game_start(self, group_id, player_score=0, opponent_score=0, current_round=1):
#         """Handle the start signal for the game."""
#         print("Game can now start!")
#
#         # Check if a game instance already exists and if the game window is still active
#         if self.game_instance and self.game_instance.game_window and self.game_instance.game_window.winfo_exists():
#             print("Game instance already exists. Starting new round.")
#             self.game_instance.game_active = True
#             self.game_instance.start_new_round()
#         else:
#             print("Game instance does not exist or was destroyed. Creating a new game instance.")
#             self.start_game(group_id, player_score, opponent_score, current_round)
#
#     def handle_round_result(self, message):
#         """Handle the result of a round."""
#         result = message.split("|")[1]
#         print(f"Round result: {result}")
#
#     def handle_opponent_disconnected(self):
#         """Handle opponent disconnection."""
#         print("Opponent disconnected. Freezing game.")
#         if self.game_instance:
#             self.game_instance.freeze_game()
#
#     def handle_opponent_reconnected(self):
#         """Handle opponent reconnection."""
#         print("Opponent reconnected. Resuming game.")
#         if self.game_instance:
#             self.game_instance.unfreeze_game()
#
#     def handle_return_to_waiting(self):
#         """Handle being sent back to the waiting screen."""
#         print("Returning to waiting screen.")
#         if self.game_instance:
#             self.game_instance.freeze_game()
#             self.game_instance.return_to_waiting_screen()
#
#     def start_game(self, group_name, player_score, opponent_score, current_round):
#         """Transition to the game when a group is assigned."""
#         print("Opponent :", self.opponent_name)
#
#         self.root.withdraw()
#
#         # Create an instance of Game with the provided group and opponent information
#         self.game_instance = Game(self.client_socket, group_name, self.player_name, self.opponent_name, self.root, player_score, opponent_score, current_round)
#
#     def cancel_waiting(self):
#         """Cancel the waiting screen and disconnect."""
#         if self.client_socket:
#             self.client_socket.close()
#         self.root.destroy()
#
#     def update_gui_safe(self, func, *args):
#         if self.root and self.root.winfo_exists():
#             self.root.after(0, func, *args)
#         else:
#             print("Root window no longer exists. Skipping GUI update.")


import tkinter as tk


class WaitingScreen:
    def __init__(self, root):
        """
        Initialize the waiting screen.

        Args:
            root (tk.Tk): The Tkinter root or Toplevel window for this screen.
        """
        self.root = root
        self.root.title("Waiting for Group Assignment")

        # Waiting message
        tk.Label(root, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)

        # Cancel button to close the waiting screen and disconnect
        cancel_button = tk.Button(root, text="Cancel", command=self.cancel_waiting)
        cancel_button.pack(pady=10)

    def cancel_waiting(self):
        """
        Handle waiting screen cancellation. This should delegate cleanup to the ServerListener.
        """
        # ServerListener should handle socket cleanup and screen transition
        print("Cancel waiting - delegate to ServerListener.")
