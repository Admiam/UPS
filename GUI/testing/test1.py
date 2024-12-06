# def listen_for_server_response(self):
#     """Listen for messages from the server to start the game."""
#     while True:
#         try:
#             # Receive a message from the server
#             response = self.client_socket.recv(1024).decode('utf-8')
#             print(f"Received message: {response}")
#
#             # Check if the response indicates successful connection
#             if response == "connected":
#                 print("Connection confirmed by server. Waiting for group assignment...")
#                 # Continue waiting for group assignment (skip to the next iteration)
#                 continue
#
#             # Check if the response is in the expected format "group_id|opponentPlayerName"
#             elif "|" in response and response.startswith("Group"):
#                 group_id, self.opponent_name = response.split("|", 1)
#                 print(f"Assigned to {group_id} with opponent {self.opponent_name}")
#                 self.start_game(group_id, 0, 0, 1)
#                 continue  # Wait for game-specific messages after starting the game
#
#             elif response.startswith("reconnect|success"):
#                 parts = response.split("|")
#                 if len(parts) == 7:  # Format: reconnect|success|opponent_name|player_score|opponent_score
#                     self.opponent_name = parts[2]
#                     player_score = int(parts[3])
#                     opponent_score = int(parts[4])
#                     current_round = int(parts[5])
#                     group_id = parts[6]
#                     print(f"Reconected with opponent", self.opponent_name, " score: ", player_score, " round: ",
#                           current_round, " group: ", group_id)
#                     self.start_game(group_id, player_score, opponent_score, current_round)
#
#                 continue
#             elif response == "error":
#                 # Handle server error
#                 messagebox.showerror("Error", "An error occurred on the server.")
#                 self.cancel_waiting()
#                 break
#
#         except Exception as e:
#             print("Connection lost:", e)
#             self.cancel_waiting()
#             break
#
# def process_server_message(self, message):
#         """Handle updates like opponent disconnection or reconnection."""
#         if message.startswith("start"):
#             print("Game can now start!")
#             self.game_active = True
#             self.start_new_round()
#         elif message.startswith("score|"):
#             parts = message.split("|")
#             if len(parts) == 3:
#                 self.p1_score = int(parts[1])
#                 self.p2_score = int(parts[2])
#                 print(f"Updated scores - Player: {self.p1_score}, Opponent: {self.p2_score}")
#                 self.update_top_bar()  # Update the UI with new scores
#         elif message.startswith("result|"):
#             result = message.split("|")[1]
#             print(f"Round result: {result}")
#         elif message.startswith("opponent_disconnected"):
#             self.freeze_game()
#         elif message.startswith("opponent_reconnected"):
#             self.unfreeze_game()
#             print("Opponent reconnected. Resuming game...")
#         elif message.startswith("return_to_waiting"):
#             self.freeze_game()
#             self.return_to_waiting_screen()