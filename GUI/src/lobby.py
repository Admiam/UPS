# # lobby.py
# import tkinter as tk
# from tkinter import messagebox
# from queue import QueueSystem  # Import your queue system
# from game import Game
#
#
# class LobbyApp:
#     def __init__(self, lobby_window):
#         self.game_window = None
#         self.lobby_window = lobby_window
#         self.queue_system = QueueSystem()  # Initialize the queue system
#
#         lobby_window.title("Group Management")
#
#         # Labels and input fields for creating a new group
#         self.label_group = tk.Label(lobby_window, text="Group Name")
#         self.label_group.grid(row=0, column=0, padx=10, pady=10)
#         self.entry_group = tk.Entry(lobby_window)
#         self.entry_group.grid(row=0, column=1, padx=10, pady=10)
#
#         # Create Group Button
#         self.button_create = tk.Button(lobby_window, text="Create Group", command=self.create_group)
#         self.button_create.grid(row=1, column=0, padx=10, pady=10)
#
#         # Add Dummy Data Button
#         self.button_dummy = tk.Button(lobby_window, text="Add Dummy Data", command=self.add_dummy_data)
#         self.button_dummy.grid(row=1, column=1, padx=10, pady=10)
#
#         # Frame to hold the list of groups and join buttons
#         self.group_frame = tk.Frame(lobby_window)
#         self.group_frame.grid(row=2, column=0, columnspan=2, padx=10, pady=10)
#
#         # Refresh the group list initially
#         self.refresh_group_list()
#
#     def add_dummy_data(self):
#         """Simulate adding dummy data to the system for testing."""
#         self.queue_system.create_group("Dummy Group 1")
#         self.queue_system.create_group("Dummy Group 2")
#         self.queue_system.join_group("Dummy Group 1", "Player A")
#         self.queue_system.join_group("Dummy Group 1", "Player B")  # This group should be full
#         self.queue_system.join_group("Dummy Group 2", "Player C")
#
#         # Refresh the UI to display the dummy data
#         self.refresh_group_list()
#         messagebox.showinfo("Dummy Data", "Added dummy groups and players for testing.")
#
#     def create_group(self):
#         group_name = self.entry_group.get()
#
#         if not group_name:
#             messagebox.showerror("Error", "Group name is required.")
#             return
#
#         self.queue_system.create_group(group_name)
#         self.refresh_group_list()
#
#     def join_group(self, group_name, player_name):
#         if not player_name:
#             messagebox.showerror("Error", "Player name is required.")
#             return
#
#         print("If result ", self.queue_system.player_in_group(player_name), "")
#         if self.queue_system.player_in_group(player_name):
#             current_group = self.queue_system.get_player_group(player_name).name
#             # messagebox.showwarning("Warning", f"{player_name} is already in group {current_group}.")
#         else:
#             # Create an instance of the Game and hide the lobby window
#             self.lobby_window.withdraw()  # Hide the lobby window
#             self.queue_system.join_group(group_name, self.entry_player.get())
#             self.create_waiting_screen(group_name, player_name)  # Call the waiting screen method
#
#     def create_waiting_screen(self, group_name, player_name):
#         """Display a waiting screen while waiting for the second player."""
#         self.waiting_window = tk.Toplevel(self.lobby_window)
#         self.waiting_window.title("Waiting for another player")
#
#         waiting_label = tk.Label(self.waiting_window, text="Waiting for another player to join...", font=("Arial", 20))
#         waiting_label.pack(padx=20, pady=50)
#
#         cancel_button = tk.Button(self.waiting_window, text="Cancel", command=self.cancel_waiting)
#         cancel_button.pack(pady=10)
#
#         # Polling to check if the second player has joined
#         self.poll_group_status(group_name, player_name)
#
#     def poll_group_status(self, group_name, player_name):
#         """Check if the second player has joined the group and start the game."""
#         player_name = self.entry_player.get()
#         self.join_group(group_name, player_name)
#         group = self.queue_system.get_group(group_name)
#
#         print(f"Number of players in group {group.players} in group {group}")
#         if len(group.players) == 2:
#             # Start the game when 2 players are in the group
#             self.waiting_window.destroy()
#             self.start_game(group_name, player_name)
#         else:
#             # Poll again after 1 second
#             self.waiting_window.after(1000, lambda: self.poll_group_status(group_name, player_name))
#
#     def cancel_waiting(self):
#         """Cancel waiting and return to the lobby."""
#         self.waiting_window.destroy()
#         self.lobby_window.deiconify()
#
#     def start_game(self, group_name, player_name):
#         """Transition to the game window when both players are ready."""
#         self.game_window = Game(self, group_name)  # Start the game
#
#     def leave_group(self):
#         player_name = self.entry_player.get()
#
#         if not player_name:
#             messagebox.showerror("Error", "Player name is required.")
#             return
#
#         if self.queue_system.player_in_group(player_name):
#             group_name = self.queue_system.get_player_group(player_name).name
#             self.queue_system.leave_group(group_name, player_name)
#             messagebox.showinfo("Success", f"{player_name} left {group_name}.")
#             self.refresh_group_list()
#         else:
#             messagebox.showwarning("Warning", f"{player_name} is not in any group.")
#
#     def refresh_group_list(self):
#         """Refresh the list of groups with join buttons."""
#         # Clear the frame before adding new buttons and labels
#
#
#         for widget in self.group_frame.winfo_children():
#             widget.destroy()
#
#         if not self.queue_system.groups:
#             tk.Label(self.group_frame, text="No groups available").grid(row=0, column=0)
#         else:
#             # Use row index to stack groups vertically
#             row = 0
#             for group in self.queue_system.groups.values():
#                 group_info = f"{group.name} - Players: {', '.join(group.players)}"
#                 if group.is_full():
#                     group_info += " (Full)"
#                 else:
#                     group_info += " (Available)"
#
#                 # Create label for each group and place it in the grid
#                 group_label = tk.Label(self.group_frame, text=group_info)
#                 group_label.grid(row=row, column=0, padx=10, pady=5, sticky="w")  # Align left
#
#                 # Create a join button for each group and place it next to the label
#                 join_button = tk.Button(
#                     self.group_frame, text="Join",
#                     command=lambda g=group.name: self.join_group(g, self.entry_player.get()),
#                     state=tk.NORMAL if not group.is_full() else tk.DISABLED
#                 )
#                 join_button.grid(row=row, column=1, padx=10, pady=5)
#
#                 # Increment row to stack groups vertically
#                 row += 1
#
#         self.group_frame.update()
#
# #
# # # Create the main window
# # lobby_window = tk.Tk()
# # app = LobbyApp(lobby_window)
# #
# # # Run the Tkinter event loop
# # lobby_window.mainloop()