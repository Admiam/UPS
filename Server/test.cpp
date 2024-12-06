#include "GameServer.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

void GameServer::add_player_to_queue(const std::string &player_id, int socket_fd)
{
    {
        std::lock_guard<std::mutex> lock(game_mutex);

        auto it = disconnected_players.find(player_id);
        if (it != disconnected_players.end() && it->second.is_reconnecting)
        {
            // Player is expected to reconnect
            std::string group_id = it->second.group_id;
            disconnected_players.erase(it);

            // Re-add player to their original group
            groups[group_id].add_player(Player(player_id, socket_fd));
            socket_to_player_id[socket_fd] = player_id; // Update mapping
            std::cout << "Reconnected player " << player_id << " to group " << group_id << "\n";
            return; // Exit without adding to queue
        }
    }

    // Add new player to the queue if they are not reconnecting
    player_queue.push(std::make_pair(player_id, socket_fd));
    socket_to_player_id[socket_fd] = player_id; // Update mapping
    std::cout << "Added player " << player_id << " to the queue\n";
    attempt_to_form_group();
}

void GameServer::attempt_to_form_group()
{
    std::cout << "Attempting to form a group\n";
    std::unique_lock<std::mutex> lock(game_mutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        std::cerr << "Failed to acquire lock on game_mutex.\n";
        return; // Exit the function if the lock is already held
    }

    std::cout << "Player queue size: " << player_queue.size() << "\n";

    // Check if we have at least two players waiting
    if (player_queue.size() < 2)
    {
        return; // Not enough players to form a group
    }

    // Get two players from the queue
    auto player1 = player_queue.front();
    player_queue.pop();
    auto player2 = player_queue.front();
    player_queue.pop();

    // Create a unique group ID
    std::string group_id = "Group " + std::to_string(next_group_number++);
    create_group(group_id);

    // Add players to the new group
    groups[group_id].add_player(Player(player1.first, player1.second));
    std::cout << "Added player1 " << player1.first << " to group " << group_id << "\n";
    groups[group_id].add_player(Player(player2.first, player2.second));
    std::cout << "Added player2 " << player2.first << " to group " << group_id << "\n";

    // Store pointers to these players in the directory
    player_directory[player1.first] = &groups[group_id].players[0];
    player_directory[player2.first] = &groups[group_id].players[1];

    std::cout << "Formed " << group_id << " with players " << player1.first << " and " << player2.first << "\n";

    // Send group information to both players
    std::string message_to_player1 = group_id + "|" + player2.first;
    std::string message_to_player2 = group_id + "|" + player1.first;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Send the message to player1
    send(player1.second, message_to_player1.c_str(), message_to_player1.size(), 0);
    std::cout << "Sent to " << player1.first << ": " << message_to_player1 << "\n";
    // Send the message to player2
    send(player2.second, message_to_player2.c_str(), message_to_player2.size(), 0);
    std::cout << "Sent to " << player2.first << ": " << message_to_player2 << "\n";
}

void GameServer::create_group(const std::string &group_id)
{
    groups.emplace(group_id, Group{group_id});
}

void GameServer::handle_disconnect(const std::string &player_id)
{
    std::lock_guard<std::mutex> lock(game_mutex);

    auto player_it = player_directory.find(player_id);
    if (player_it == player_directory.end())
    {
        return; // Player not found
    }

    Player *player = player_it->second;
    player->is_connected = false;
    socket_to_player_id.erase(player->socket_fd); // Remove the socket mapping
    std::cout << "Player " << player_id << " disconnected\n";

    // Find the group containing this player
    for (auto &[group_id, group] : groups)
    {
        if (group.players.size() < 2)
            continue;

        if (group.players[0].player_id == player_id || group.players[1].player_id == player_id)
        {
            // Start a reconnection timer for the group
            std::thread(&GameServer::start_reconnection_timer, this, group_id, player_id).detach();
            break;
        }
    }
}

void GameServer::start_reconnection_timer(const std::string &group_id, const std::string &player_id)
{
    {
        std::lock_guard<std::mutex> lock(game_mutex);
        disconnected_players[player_id] = {group_id, true, std::chrono::steady_clock::now()};
    }

    // Wait for the reconnection window (e.g., 10 seconds)
    std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        std::lock_guard<std::mutex> lock(game_mutex);
        auto it = disconnected_players.find(player_id);
        if (it != disconnected_players.end() && it->second.is_reconnecting)
        {
            std::cout << "Reconnection timeout for player " << player_id << " in group " << group_id << "\n";
            handle_reconnection_timeout(group_id, player_id);
            disconnected_players.erase(it);
        }
    }
}

void GameServer::handle_reconnection_timeout(const std::string &group_id, const std::string &player_id)
{
    std::lock_guard<std::mutex> lock(game_mutex);

    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cout << "Group not found\n";
        return;
    }

    Group &group = group_it->second;

    // Check if the player is still disconnected
    auto it = std::find_if(group.players.begin(), group.players.end(),
                           [&](const Player &p)
                           { return p.player_id == player_id; });
    if (it != group.players.end() && !it->is_connected)
    {
        // Remove the disconnected player from the group
        group.remove_player(player_id);
        player_directory.erase(player_id);

        // Return the remaining player to the queue if there is one
        if (group.players.size() == 1)
        {
            auto remaining_player = group.players[0];
            player_queue.push(std::make_pair(remaining_player.player_id, remaining_player.socket_fd));
            std::cout << "Returning player " << remaining_player.player_id << " to the queue\n";
        }

        // Remove the group if empty
        if (group.players.empty())
        {
            groups.erase(group_id);
        }

        // Attempt to form a new group if there are enough players in the queue
        attempt_to_form_group();
    }
}

int GameServer::get_socket_fd_for_player(const std::string &player_id) const
{
    for (const auto &entry : socket_to_player_id)
    {
        if (entry.second == player_id)
        {
            std::cout << "Player ID: " << player_id << " Socket FD: " << entry.first << "\n";
            return entry.first; // Return the file descriptor if the player_id matches
        }
    }
    return -1; // Return -1 if the player ID is not found
}

std::string GameServer::get_player_group(const std::string &player_id)
{
    for (const auto &group : groups)
    {
        const auto &players = group.second.players;

        // Use std::find_if to check if the player exists in the vector
        auto it = std::find_if(players.begin(), players.end(),
                               [&player_id](const Player &p)
                               { return p.player_id == player_id; });

        if (it != players.end())
        {
            return group.first; // Return the group ID if the player is found
        }
    }
    return ""; // Player not found in any group
}

bool GameServer::register_choice(const std::string &group_id, const std::string &player_id, const std::string &choice)
{
    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cerr << "Group " << group_id << " not found.\n";
        return false;
    }

    Group &group = group_it->second;

    // Register the player's choice
    group.choices[player_id] = choice;

    // Check if all players in the group have made their choices
    return group.choices.size() == group.players.size();
}

void GameServer::process_group_round(const std::string &group_id)
{
    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cerr << "Group " << group_id << " not found.\n";
        return;
    }

    Group &group = group_it->second;

    // Ensure there are exactly two players in the group
    if (group.players.size() != 2)
    {
        std::cerr << "Invalid group size for " << group_id << "\n";
        return;
    }

    auto it = group.players.begin();
    const std::string &player1_id = it->player_id;
    const std::string &player2_id = (++it)->player_id;

    const std::string &player1_choice = group.choices[player1_id];
    const std::string &player2_choice = group.choices[player2_id];

    PlayerMove player1{player1_id, player1_choice};
    PlayerMove player2{player2_id, player2_choice};

    // Process the round using GameLogic
    group.logic.process_round(player1, player2);

    // Clear choices for the next round
    group.reset_choices();

    // Send updated scores to both players
    int player1_fd = get_socket_fd_for_player(player1_id);
    int player2_fd = get_socket_fd_for_player(player2_id);

    int player1_score = group.logic.get_score(player1_id);
    int player2_score = group.logic.get_score(player2_id);

    // Construct messages
    std::string player1_score_msg = "score|" + std::to_string(player1_score) + "|" + std::to_string(player2_score);
    std::string player2_score_msg = "score|" + std::to_string(player2_score) + "|" + std::to_string(player1_score);

    std::cout << "Player 1 socket: " << player1_fd << "\n";
    // Send scores to both players
    send(player1_fd, player1_score_msg.c_str(), player1_score_msg.size(), 0);
    send(player2_fd, player2_score_msg.c_str(), player2_score_msg.size(), 0);

    std::cout << "Score sent to " << player1_id << ": " << player1_score_msg << "\n";
    std::cout << "Score sent to " << player2_id << ": " << player2_score_msg << "\n";
    std::cout << "Round processed for group " << group_id << ".\n";
}