# # queue_system.py
#
# class Group:
#     def __init__(self, name):
#         self.name = name
#         self.players = []
#
#     def add_player(self, player):
#         if len(self.players) < 2:
#             self.players.append(player)
#             print(f"{player} joined the group {self.name}.")
#         else:
#             print(f"Group {self.name} is full. Player {player} cannot join.")
#
#     def remove_player(self, player):
#         if player in self.players:
#             self.players.remove(player)
#             print(f"{player} left the group {self.name}.")
#         else:
#             print(f"{player} is not in the group {self.name}.")
#
#     def is_full(self):
#         return len(self.players) == 2
#
#     def has_player(self, player):
#         return player in self.players
#
#     def __str__(self):
#         if len(self.players) == 2:
#             status = "Full"
#         elif len(self.players) == 1:
#             status = "Available (1 spot left)"
#         else:
#             status = "Available (2 spots left)"
#         return f"Group: {self.name}, Players: {', '.join(self.players)}, Status: {status}"
#
#
# class QueueSystem:
#     def __init__(self):
#         self.groups = {}
#         self.player_group_map = {}  # Map player names to their current group
#
#         self.create_dummy_data()
#
#     def create_group(self, group_name):
#         if group_name not in self.groups:
#             self.groups[group_name] = Group(group_name)
#             print(f"Group {group_name} created.")
#         else:
#             print(f"Group {group_name} already exists.")
#
#     def join_group(self, group_name, player):
#         if player in self.player_group_map:
#             print(
#                 f"{player} is already in a group ({self.player_group_map[player].name}). Leave the current group first.")
#             return
#
#         if group_name in self.groups:
#             group = self.groups[group_name]
#             if not group.is_full():
#                 group.add_player(player)
#                 self.player_group_map[player] = group
#             else:
#                 print(f"Group {group_name} is full.")
#         else:
#             print(f"Group {group_name} does not exist.")
#
#     def leave_group(self, group_name, player):
#         if player in self.player_group_map:
#             group = self.player_group_map[player]
#             if group.name == group_name:
#                 group.remove_player(player)
#                 del self.player_group_map[player]
#             else:
#                 print(f"{player} is not in group {group_name}.")
#         else:
#             print(f"{player} is not in any group.")
#
#     def display_groups(self):
#         if not self.groups:
#             print("No groups created yet.")
#         else:
#             for group in self.groups.values():
#                 print(group)
#
#     def player_in_group(self, player):
#         return player in self.player_group_map
#
#     def get_player_group(self, player):
#         return self.player_group_map.get(player, None)
#
#     def get_group(self, group_name):
#         """Retrieve a group by name."""
#         return self.groups.get(group_name)
#
#     def create_dummy_data(self):
#         # Create some dummy groups and players
#         self.create_group("Test Group 1")
#         self.create_group("Test Group 2")
#         self.join_group("Test Group 1", "Alice")
#         self.join_group("Test Group 1", "Bob")  # Should fill Test Group 1
#         self.join_group("Test Group 2", "Charlie")
#
#         print("-------- Dummy data created with 2 groups and 3 players.--------")
