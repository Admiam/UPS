#include "GameServer.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>

void GameServer::add_player_to_queue(const std::string &player_id, int socket_fd)
{
    // std::cout << "DEBUG > Adding player to queue: " << player_id << " with socket FD: " << socket_fd << "\n";
    // {
    //     std::lock_guard<std::mutex> lock(game_mutex);
    // std::cout << "DEBUG > KOKOT1 \n";
    auto it = disconnected_players.find(player_id);
    // std::cout << "DEBUG > KOKOT2 \n";

    std::cout << "-----------Add player to queue before-----------\n";
    print_disconnected_players();
    print_groups();
    print_player_queue();
    std::cout << "-----------Add player to queue before-----------\n";

    if (it != disconnected_players.end() && it->second.is_reconnecting)
    {

        // Player is expected to reconnect
        std::string group_id = it->second.group_id;
        disconnected_players.erase(it);

        // Re-add player to their original group
        groups[group_id].add_player(Player(player_id, socket_fd));
        socket_to_player_id[socket_fd] = player_id; // Update mapping
        std::cout << "Reconnected player " << player_id << " to group " << group_id << "\n";

        // Get the opponent player and current scores
        auto &group = groups[group_id];
        std::string opponent_name;
        int player_score = 0;
        int opponent_score = 0;
        int current_round = group.logic.get_current_round(); // Assuming GameLogic tracks the current round

        for (const auto &player : group.players)
        {
            if (player.player_id != player_id)
            {
                opponent_name = player.player_id;
                opponent_score = group.logic.get_score(opponent_name); // Assuming GameLogic tracks scores
            }
            else
            {
                player_score = group.logic.get_score(player_id);
            }
        }

        // std::cout << "---------Add player to queue mid-- -- -- -- -\n ";
        // print_disconnected_players();
        // print_groups();
        // print_player_queue();
        // std::cout << "---------Add player to queue mid-- -- -- -- -\n ";

        // Notify the reconnecting player about their opponent, scores, and that the game can resume
        std::string resume_message = "RPS|reconnect|success|" + opponent_name +
                                        "|" + std::to_string(player_score) +
                                        "|" + std::to_string(opponent_score) +
                                        "|" + std::to_string(current_round) +
                                        "|" + group_id + ";";

        send(socket_fd, resume_message.c_str(), resume_message.size(), 0);
        std::cout << "Sent resume message to " << player_id << ": " << resume_message << "\n";
        std::cout << "SERVER > send: " << resume_message << "\n";

        // Notify the opponent that the player has reconnected
        for (const auto &player : group.players)
        {
            if (player.player_id != player_id)
            {
                int other_fd = player.socket_fd;
                std::string reconnect_message = "RPS|reconnect|" + player_id + "|game_resume" + ";";
                send(other_fd, reconnect_message.c_str(), reconnect_message.size(), 0);
                std::cout << "RECONNECT > Notified " << player.player_id << " of reconnection: " << reconnect_message << "\n";
                std::cout << "SERVER > send: " << reconnect_message << "\n";
            }
        }
    }
    // }

    // update_player_state(player_id, LOBBY);
    // Add new player to the queue if they are not reconnecting
    player_queue.push(std::make_pair(player_id, socket_fd));

    socket_to_player_id[socket_fd] = player_id; // Update mapping

    // std::cout << "DEBUG > -----------Add player to queue after----------- \n ";
    // print_disconnected_players();
    // print_groups();
    // print_player_queue();
    // std::cout << "DEBUG > -----------Add player to queue after----------- \n ";

    // return; // Exit without adding to the queue

    std::cout << "LOBBY > Added player " << player_id << " to the queue\n";
    attempt_to_form_group();
}

void GameServer::attempt_to_form_group()
{
    // std::cout << "Attempting to form a group\n";
    // std::unique_lock<std::mutex> lock(game_mutex, std::try_to_lock);

    print_player_queue();

    // if (!lock.owns_lock())
    // {
    //     std::cerr << "ERROR > Failed to acquire lock on game_mutex.\n";
    //     return; // Exit the function if the lock is already held
    // }

    if (player_queue.size() < 2)
    {
        std::cout << "LOBBY > Not enough players to form a group.\n";
        return;
    }

    auto player1 = player_queue.front();
    player_queue.pop();
    auto player2 = player_queue.front();
    player_queue.pop();

    std::string group_id = "Group " + std::to_string(next_group_number++);
    create_group(group_id);

    groups[group_id].add_player(Player(player1.first, player1.second));
    groups[group_id].add_player(Player(player2.first, player2.second));
    // std::cout << "Formed " << group_id << " with players " << player1.first << " and " << player2.first << "\n";

    std::string message_to_player1 = "RPS|" + group_id + "|" + player2.first + ";";
    std::string message_to_player2 = "RPS|" + group_id + "|" + player1.first + ";";


    // player_directory[player1.first] = &groups[group_id].players.back();
    // player_directory[player2.first] = &groups[group_id].players.back();

    player_directory[player1.first] = &groups[group_id].players[0];
    player_directory[player2.first] = &groups[group_id].players[1];

    send(player1.second, message_to_player1.c_str(), message_to_player1.size(), 0);
    std::cout << "SERVER SENT > " << message_to_player1 << "\n";

    send(player2.second, message_to_player2.c_str(), message_to_player2.size(), 0);
    std::cout << "SERVER SENT > " << message_to_player2 << "\n";

    print_groups();

    // std::cout << "Sent group assignment to both players.\n";
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
        std::cout << "ERROR > player: " << player_id << " not fount \n";
        return; // Player not found
    }

    Player *player = player_it->second;
    player->is_connected = false;
    socket_to_player_id.erase(player->socket_fd); // Remove the socket mapping
    std::cout << "DISCONNECT > player " << player_id << " disconnected\n";

    for (auto &[group_id, group] : groups)
    {
        if (group.contains_player(player_id))
        {
            group.remove_player(player_id);
            disconnected_players[player_id] = {group_id, true, std::chrono::steady_clock::now()};
            // std::cout << "DEBUG > -----------Handle disconnect player before-----------\n";
            print_disconnected_players();
            print_groups();
            print_player_queue();
            // std::cout << "DEBUG > -----------Handle disconnect player before-----------\n";

            if (group.players.empty())
            {
                groups.erase(group_id);
                std::cout << "DISCONNECT > Group " << group_id << " removed as all players are disconnected.\n";
            }
            else
            {
                std::cout << "RECONNECT > Starting reconnection timer for player " << player_id << " in group " << group_id << "\n";
                update_player_state(player_id, RECONNECTING); // Set the player's state to RECONNECTING
                std::thread(&GameServer::start_reconnection_timer, this, group_id, player_id).detach();
            }
            // std::cout << "DEBUG > ----------Handle disconnect player after----------\n";
            print_disconnected_players();
            print_groups();
            print_player_queue();
            // std::cout << "DEBUG > ----------Handle disconnect player after----------\n";

            break;
        }
    }

    player_directory.erase(player_id); // Remove player from directory
}

void GameServer::handle_internet_disconnect(const std::string &player_id)
{
    std::cout << "DEBUG > Player " << player_id << " disconnected due to internet issues.\n";
    Player *player = get_player_by_id(player_id); //TODO segmentation fault
    player->is_connected = false;

    if (player->state == LOBBY)
    {
        std::cout << "DEBUG > player " << player_id << " was removed from queue " << remove_player_from_queue(player_id) << "\n";
    }

    update_player_state(player_id, RECONNECTING); // Update state to LOBBY
    std::string group_id = get_player_group(player_id);
    if (group_id.empty())
    {
        std::cerr << "ERROR > Player " << player_id << " is not part of any group.\n";
        return;
    }
    // Spusťte nový thread pro oznámení soupeřům
    std::thread([this, player_id, group_id]()
                { notify_opponent_disconnected(player_id, group_id); })
        .detach();


    // Přidejte hráče do seznamu odpojených a začněte čekat na znovupřipojení
    // {
    //     std::lock_guard<std::mutex> lock(game_mutex);
    disconnected_players[player_id] = {group_id, true, std::chrono::steady_clock::now()};
    // }

    // Spusťte timer pro znovupřipojení
    std::thread(&GameServer::start_reconnection_timer, this, group_id, player_id).detach();
}

void GameServer::start_reconnection_timer(const std::string &group_id, const std::string &player_id)
{
    std::cout << "DEBUG > start_reconnection_timer started for player " << player_id << " in group " << group_id << "\n";

    disconnected_players[player_id] = {group_id, true, std::chrono::steady_clock::now()};

    std::this_thread::sleep_for(std::chrono::seconds(30)); // Example timeout duration

    std::lock_guard<std::mutex> lock(game_mutex);
    auto it = disconnected_players.find(player_id);
    if (it != disconnected_players.end() && it->second.is_reconnecting)
    {
        // std::cout << "DEBUG > Reconnection timeout for player " << player_id << "\n";
        disconnected_players.erase(it);
        handle_reconnection_timeout(group_id, player_id);
        // std::cout << "DEBUG > kokot1\n";
        // std::cout << "DEBUG > kokot2\n";
    }
}

void GameServer::handle_reconnection_timeout(const std::string &group_id, const std::string &player_id)
{
    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cerr << "ERROR > Group not found for player " << player_id << "\n";
        update_player_state(player_id, LOBBY); // Update state to LOBBY
        return;
    }

    Group &group = group_it->second;

    // Prepare to check the connection status of both players
    std::vector<std::string> players_to_remove;
    std::vector<std::string> connected_players;

    group.remove_player(player_id);
    // Check each player's status and prepare to remove disconnected players
    for (const auto &player : group.players)
    {
        if (player.player_id == player_id || !player.is_connected || disconnected_players.find(player.player_id) != disconnected_players.end())
        {
            players_to_remove.push_back(player.player_id);
            update_player_state(player.player_id, LOBBY); // Update state to LOBBY
        }
        else
        {
            connected_players.push_back(player.player_id); // Collect connected players
        }
    }

    // Notify connected players and prepare for their return to the lobby
    for (const auto &opponent_id : connected_players)
    {
        int opponent_fd = get_socket_fd_for_player(opponent_id);
        std::string return_to_waiting_msg = "RPS|return_to_waiting;";
        send(opponent_fd, return_to_waiting_msg.c_str(), return_to_waiting_msg.size(), 0);
        std::cout << "NOTIFY > Opponent " << opponent_id << " sent to waiting screen due to timeout.\n" ;
        group.remove_player(opponent_id);
        handle_return_to_lobby(opponent_id);
    }

    // Remove both players from the group
    for (const auto &pid : players_to_remove)
    {
        group.remove_player(pid);
        std::cout << "GROUP > Removed player " << pid << " from group " << group_id << ".\n";
    }

    // If the group becomes empty or if both players were disconnected, remove it
    if (group.players.empty() || connected_players.empty())
    {
        groups.erase(group_id);
        std::cout << "GROUP > Group " << group_id << " has been removed as all players are disconnected.\n";
    }
}

void GameServer::handle_reconnection(const std::string &player_id, int new_fd)
{
    // std::lock_guard<std::mutex> lock(game_mutex);

    print_disconnected_players();
    auto it = disconnected_players.find(player_id);
    if (it == disconnected_players.end())
    {
        std::cerr << "ERROR > Reconnection failed: Player " << player_id << " not in disconnected list.\n";
        return;
    }

    std::string group_id = it->second.group_id;
    disconnected_players.erase(it); // Remove from disconnected players

    // Update the player's socket FD and connection status
    auto group_it = groups.find(group_id);
    if (group_it != groups.end())
    {
        Group &group = group_it->second;

        for (auto &player : group.players)
        {
            if (player.player_id == player_id)
            {
                // Remove old socket_fd mapping if it exists
                for (auto socket_it = socket_to_player_id.begin(); socket_it != socket_to_player_id.end(); ++socket_it)
                {
                    if (socket_it->second == player_id)
                    {
                        socket_to_player_id.erase(socket_it);
                        break;
                    }
                }

                player.socket_fd = new_fd;
                player.is_connected = true;

                // Add new socket_fd mapping
                socket_to_player_id[new_fd] = player_id;

                break;
            }
        }
    }
    else
    {
        std::cerr << "ERROR > Group " << group_id << " not found for player " << player_id << "\n";
    }

    std::cout << "RECONNECT > Player " << player_id << " reconnected successfully with new FD: " << new_fd << "\n";
}

int GameServer::get_socket_fd_for_player(const std::string &player_id) const
{
    // std::cout << "Player ID: " << player_id << "\n";
    for (const auto &entry : socket_to_player_id)
    {

        // std::cout << "Entry first: " << entry.first << " vs player id: " << player_id << "\n";
        // std::cout << "Entry second: " << entry.second << " vs player id: " << player_id << "\n";

        if (entry.second == player_id)
        {
            // std::cout << "Player ID: " << player_id << " Socket FD: " << entry.first << "\n";
            return entry.first; // Return the file descriptor if the player_id matches
        }
    }
    return -1; // Return -1 if the player ID is not found
}

std::string GameServer::extract_payload(const std::string &message)
{
    if (message.size() <= 4)
    {
        return ""; // Invalid message, return an empty string
    }
    return message.substr(4); // Skip the first 4 bytes
}

std::string GameServer::normalize_string(const std::string &str)
{
    std::string normalized;
    for (size_t i = 0; i < str.size(); ++i)
    {
        // Check if the current and next byte represent \u00A0 in UTF-8
        if (i + 1 < str.size() && static_cast<unsigned char>(str[i]) == 0xC2 &&
            static_cast<unsigned char>(str[i + 1]) == 0xA0)
        {
            normalized += ' '; // Replace non-breaking space with a regular space
            ++i;               // Skip the next byte
        }
        else
        {
            normalized += str[i];
        }
    }
    return normalized;
}

std::string GameServer::trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return ""; // String is all whitespace

    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string GameServer::get_player_group(const std::string &player_id) 
{
    std::cout << "DEBUG > Searching for player ID: " << player_id << " in groups.\n";

    for (const auto &group : groups)
    {
        // std::cout << "DEBUG > Checking group ID: " << group.first << "\n";
        const auto &players = group.second.players;

        std::cout << "DEBUG > Players in group " << group.first << ": ";
        for (const auto &player : players)
        {
            std::cout << player.player_id << " ";
            if (normalize_string(trim(extract_payload(player_id))) == normalize_string(trim(extract_payload(player.player_id)))){
                std::cout << "DEBUG > Return group " << group.first << "\n";
                return group.first;
            }
        }
        std::cout << "\n";

        // Use std::find_if to check if the player exists in the vector
        auto it = std::find_if(players.begin(), players.end(),
                               [&player_id](const Player &p)
                               { return p.player_id == player_id; });

        if (it != players.end())
        {
            // std::cout << "DEBUG > Found player ID: " << player_id << " in group ID: " << group.first << "\n";
            return group.first; // Return the group ID if the player is found
        }
    }

    // std::cout << "DEBUG > Player ID: " << player_id << " not found in any group.\n";
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

    // Increment the round counter for the group
    group.logic.increment_round();

    // Clear choices for the next round
    group.reset_choices();

    // Send updated scores to both players
    int player1_fd = get_socket_fd_for_player(player1_id);
    int player2_fd = get_socket_fd_for_player(player2_id);

    int player1_score = group.logic.get_score(player1_id);
    int player2_score = group.logic.get_score(player2_id);
    int current_round = group.logic.get_current_round();

    // Construct messages
    std::string player1_score_msg = "RPS|score|" + std::to_string(player1_score) + "|" + std::to_string(player2_score) + ";";
    std::string player2_score_msg = "RPS|score|" + std::to_string(player2_score) + "|" + std::to_string(player1_score) + ";";

    // std::cout << "Player 1 socket: " << player1_fd << "\n";
    // Send scores to both players
    send(player1_fd, player1_score_msg.c_str(), player1_score_msg.size(), 0);
    std::cout << "SERVER > send: " << player1_score_msg << " on socked fd: "  << player1_fd << "\n";
    send(player2_fd, player2_score_msg.c_str(), player2_score_msg.size(), 0);
    std::cout << "SERVER > send: " << player2_score_msg << " on socked fd: " << player2_fd << "\n";
}

void GameServer::handle_ready_message(const std::string &player_id)
{
    std::string group_id = get_player_group(player_id);
    if (group_id.empty())
    {
        std::cerr << "Player " << player_id << " is not part of any group.\n";
        return;
    }

    mark_player_ready(group_id, player_id);
}

void GameServer::notify_players_to_start(const Group &group)
{
    for (const Player &player : group.players)
    {
        int socket_fd = get_socket_fd_for_player(player.player_id);
        std::string start_message = "RPS|start;";
        send(socket_fd, start_message.c_str(), start_message.size(), 0);
        std::cout << "SERVER > send: " << start_message << "\n";
        std::cout << "GAME > Sent start message to " << player.player_id << "\n";
    }
}

void GameServer::mark_player_ready(const std::string &group_id, const std::string &player_id)
{
    std::lock_guard<std::mutex> lock(game_mutex);

    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cerr << "ERROR > Group " << group_id << " not found.\n";
        return;
    }

    Group &group = group_it->second;
    group.ready_state[player_id] = true; // Mark the player as ready

    std::cout << "GAME > Player " << player_id << " is ready in group " << group_id << "\n";

    // Check if all players in the group are ready
    if (std::all_of(group.players.begin(), group.players.end(),
                    [&group](const Player &p)
                    {
                        return group.ready_state[p.player_id];
                    }))
    {
        notify_players_to_start(group);
    }
}

void GameServer::handle_return_to_lobby(const std::string &player_id)
{
    bool player_removed = true;

    // // Remove the player from any active group
    // for (auto &[group_id, group] : groups)
    // {
    //     if (group.contains_player(player_id))
    //     {
    //         group.remove_player(player_id);
    //         std::cout << "DISCONNECT > Removed player " << player_id << " from group " << group_id << ".\n";   

    //         player_removed = true;
    //         break; // Stop iterating as the player is already removed
    //     }
    // }

    if (!player_removed)
    {
        std::cerr << "WARNING > Player " << player_id << " was not part of any group.\n";
    }

    // Find the socket_fd associated with player_id
    int socket_fd = get_socket_fd_for_player(player_id);
    if (socket_fd != -1)
    {
        // Add the player back to the queue
        update_player_state(player_id, LOBBY); // Update state to LOBBY

        add_player_to_queue(player_id, socket_fd);
        std::cout << "LOBBY > Player " << player_id << " has been added back to the lobby.\n";
    }
    else
    {
        std::cerr << "ERROR > Could not find socket_fd for player_id " << player_id << "\n";
    }
}

void GameServer::print_to_hex(const std::string &str) const
{
    for (unsigned char c : str)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << "\n";
}

void GameServer::print_player_queue() const
{
    std::cout << "\n========== Player Queue ==========\n";
    std::queue<std::pair<std::string, int>> temp_queue = player_queue; // Copy the queue for iteration

    if (temp_queue.empty())
    {
        std::cout << "No players in the queue.\n";
    }
    else
    {
        while (!temp_queue.empty())
        {
            auto player = temp_queue.front();
            temp_queue.pop();
            std::cout << "Player ID: " << std::setw(15) << std::left << player.first
                      << "Socket FD: " << player.second << "\n";
        }
    }
    // std::cout << "===================================\n";
}

void GameServer::print_groups() const
{
    std::cout << "\n========== Current Groups ==========\n";

    if (groups.empty())
    {
        std::cout << "No active groups.\n";
    }
    else
    {
        for (const auto &[group_id, group] : groups)
        {
            std::cout << "Group ID: " << group_id << "\n";
            for (const auto &player : group.players)
            {
                std::cout << "  Player ID: " << std::setw(15) << std::left << player.player_id
                          << "Socket FD: " << std::setw(5) << player.socket_fd
                          << "Connected: " << (player.is_connected ? "Yes" : "No") << "\n";
            }
        }
    }

    // std::cout << "=====================================\n";
}

void GameServer::print_disconnected_players() const
{
    std::cout << "\n========== Disconnected Players ==========\n";

    if (disconnected_players.empty())
    {
        std::cout << "No disconnected players.\n";
    }
    else
    {
        for (const auto &[player_id, info] : disconnected_players)
        {
            std::cout << "Player ID: " << std::setw(15) << std::left << player_id
                      << "Group ID: " << std::setw(10) << info.group_id
                      << "Reconnecting: " << (info.is_reconnecting ? "Yes" : "No") << "\n";
        }
    }

    // std::cout << "==========================================\n";
}

void GameServer::update_ping(const std::string &player_id)
{
    std::lock_guard<std::mutex> lock(game_mutex);

    auto it = disconnected_players.find(player_id);
    if (it != disconnected_players.end())
    {
        // Clear reconnection state
        it->second.is_reconnecting = false;
        disconnected_players.erase(it); // Remove from disconnected list
        std::cout << "RECONNECT > Player " << player_id << " marked as connected.\n";
    }
    // Update the last ping time

    player_last_ping[player_id] = std::chrono::steady_clock::now();
}

void GameServer::check_for_timeouts()
{
    std::lock_guard<std::mutex> lock(game_mutex);
    auto now = std::chrono::steady_clock::now();
    for (auto it = player_last_ping.begin(); it != player_last_ping.end();)
    {
        const std::string &player_id = it->first;
        auto time_since_last_ping = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();

        if (time_since_last_ping > 3)
        {
            std::cout << "RECONNECT > Player " << player_id << " timed out.\n";
            handle_internet_disconnect(player_id);
            it = player_last_ping.erase(it); // TODO neostranovat hrace pokud se pripoji
        }
        else
        {
            ++it;
        }
    }
}

void GameServer::disconnect_player_due_to_timeout(const std::string &player_id)
{
    std::cout << "DISCONNECT > Disconnecting player due to timeout: " << player_id << "\n";
    handle_disconnect(player_id); // Existing method for disconnection
}

void GameServer::cleanup()
{
    std::lock_guard<std::mutex> lock(game_mutex); // Ensure thread safety
    groups.clear();                               // Clear all game groups
    player_queue = {};                            // Clear the player queue
    socket_to_player_id.clear();                  // Clear socket-to-player mappings
    disconnected_players.clear();                 // Clear disconnected players
    // std::cout << "Game server resources cleaned up.\n";
}

bool GameServer::is_player_reconnecting(const std::string &player_id) const
{
    // std::lock_guard<std::mutex> lock(game_mutex); // Ensure thread safety
    auto it = disconnected_players.find(player_id);
    return it != disconnected_players.end() && it->second.is_reconnecting;
}

void GameServer::notify_opponent_reconnected(const std::string &player_id)
{
    std::string group_id = get_player_group(player_id);
    if (group_id.empty())
    {
        std::cerr << "ERROR > Player " << player_id << " is not part of any group.\n";
        return;
    }

    Group &group = groups[group_id];

    // Message to notify players
    std::string success_msg = "RPS|opponent_reconnected;";

    for (const auto &player : group.players)
    {
        if (player.player_id != player_id) // Notify the opponent
        {
            int opponent_fd = player.socket_fd;
            send(opponent_fd, success_msg.c_str(), success_msg.size(), 0);
            std::cout << "RECONNECT > Notified " << player.player_id << " about reconnection of " << player_id << ".\n";
            std::cout << "SERVER > send: " << success_msg << "\n";
        }
        else // Optionally, notify the reconnected player
        {
            int reconnected_fd = player.socket_fd;
            send(reconnected_fd, success_msg.c_str(), success_msg.size(), 0);
            std::cout << "RECONNECT > Player " << player_id << " with new socket FD: " << reconnected_fd << " notified.\n";
            std::cout << "SERVER > send: " << success_msg << "\n";
        }
    }
}

void GameServer::notify_opponent_disconnected(const std::string &player_id, const std::string &group_id)
{
    Group &group = groups[group_id];
    for (const auto &player : group.players)
    {
        if (player.player_id != player_id)
        {
            int fd = get_socket_fd_for_player(player.player_id);
            std::string message = "RPS|opponent_disconnected;";
            send(fd, message.c_str(), message.size(), 0);
            std::cout << "DISCONNECT > Notified " << player.player_id << " about disconnection of " << player_id << ".\n";
            std::cout << "SERVER > send: " << message << "\n";
        }
    }
}

void GameServer::check_for_inactive_players()
{
    std::lock_guard<std::mutex> lock(game_mutex);
    auto now = std::chrono::steady_clock::now();

    for (auto it = player_last_ping.begin(); it != player_last_ping.end();)
    {
        const std::string &player_id = it->first;
        auto time_since_last_ping = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();

        if (time_since_last_ping > 3) // Define a timeout period
        {
            std::cout << "RECONNECT > Player " << player_id << " timed out.\n";
            handle_internet_disconnect(player_id); // Notify and handle timeout
            it = player_last_ping.erase(it);       // Remove the player from ping tracking
        }
        else
        {
            ++it;
        }
    }
}

// void GameServer::handle_invalid_message(const std::string &player_id)
// {
//     std::cout << "INVALID MESSAGE > Disconnecting player " << player_id << ".\n";

//     // Find the group and notify the opponent
//     std::string group_id = get_player_group(player_id);
//     if (!group_id.empty())
//     {
//         Group &group = groups[group_id];

//         // Notify the opponent to return to the lobby
//         for (const auto &player : group.players)
//         {
//             if (player.player_id != player_id)
//             {
//                 int opponent_fd = get_socket_fd_for_player(player.player_id);
//                 if (opponent_fd != -1)
//                 {
//                     std::string msg = "RPS|return_to_waiting;";
//                     send(opponent_fd, msg.c_str(), msg.size(), 0);
//                     std::cout << "NOTIFY > Opponent " << player.player_id << " sent to lobby.\n";
//                 }
//             }
//         }

//         // Remove the player from the group
//         group.remove_player(player_id);
//         std::cout << "GROUP CLEANUP > Removed player " << player_id << " from group " << group_id << ".\n";

//         // Remove the group if empty
//         if (group.players.empty())
//         {
//             groups.erase(group_id);
//             std::cout << "GROUP CLEANUP > Group " << group_id << " has been removed.\n";
//         }
//     }
//     else
//     {
//         std::cerr << "ERROR > Player " << player_id << " is not part of any group.\n";
//     }
// }

Player *GameServer::get_player_by_id(const std::string &player_id)
{
    // First check in groups for the player
    for (auto &[group_id, group] : groups)
    {
        for (auto &player : group.players)
        {
            if (player.player_id == player_id)
            {
                return &player; // Return a pointer to the Player object
            }
        }
    }

    // If the player is not found in any group, check the queue
    std::queue<std::pair<std::string, int>> temp_queue = player_queue;
    while (!temp_queue.empty())
    {
        auto player_info = temp_queue.front();
        temp_queue.pop();
        if (player_info.first == player_id)
        {
            // Assuming you have a map or method to get Player objects from player IDs in the queue
            return new Player(player_info.first, player_info.second); // Create a new Player object or fetch it if stored elsewhere
        }
    }

    // If the player is not found in groups or the queue
    return nullptr;
}

void GameServer::update_player_state(const std::string &player_id, PlayerState new_state)
{
    Player *player = get_player_by_id(player_id);
    if (player)
    {
        player->state = new_state;
        std::cout << "STATE > Player " << player_id << " state updated to " << new_state << ".\n";
    }
}

bool GameServer::remove_player_from_queue(const std::string &player_id)
{
    std::lock_guard<std::mutex> lock(game_mutex); // Lock to ensure thread safety

    std::queue<std::pair<std::string, int>> new_queue;
    bool removed = false;

    // Iterate through the current queue and rebuild it without the specified player
    while (!player_queue.empty())
    {
        auto current = player_queue.front();
        player_queue.pop();
        if (current.first == player_id)
        {
            std::cout << "Player " << player_id << " removed from the queue.\n";
            removed = true; // Mark as removed
            return true;
        }
        else
        {
            new_queue.push(current); // Keep other players in the new queue
        }
    }

    std::swap(player_queue, new_queue); // Swap the old queue with the new one

    if (!removed)
    {
        std::cerr << "Player " << player_id << " not found in the queue.\n";
    }
    return false;

    // No need to check other states as we only focus on queue management here
}

void GameServer::reset_and_remove_player(const std::string &player_id)
{
    std::string opponent_id;

    // Remove from the player queue
    if (remove_player_from_queue(player_id))
    {
        return;       
    }

    std::string group_id = get_player_group(player_id);
    if (group_id.empty())
    {
        std::cerr << "ERROR > Player " << player_id << " is not part of any group.\n";
        return;
    }

    auto group_it = groups.find(group_id);
    if (group_it == groups.end())
    {
        std::cerr << "ERROR > Group not found for player " << player_id << "\n";
        // update_player_state(player_id, LOBBY); // Update state to LOBBY
        return;
    }

    Group &group = group_it->second;

    // Prepare to check the connection status of both players
    std::vector<std::string> players_to_remove;
    std::vector<std::string> connected_players;

    group.remove_player(player_id);
    // Check each player's status and prepare to remove disconnected players
    for (const auto &player : group.players)
    {
        if (player.player_id == player_id || !player.is_connected || disconnected_players.find(player.player_id) != disconnected_players.end())
        {
            players_to_remove.push_back(player.player_id);
            update_player_state(player.player_id, LOBBY); // Update state to LOBBY
        }
        else
        {
            connected_players.push_back(player.player_id); // Collect connected players
        }
    }

    // Remove from disconnected players map
    disconnected_players.erase(player_id);

    // Clear any specific player data if stored in other structures
    socket_to_player_id.erase(get_socket_fd_for_player(player_id));
    player_directory.erase(player_id);

    for (const auto &opponent_id : connected_players)
    {
        int opponent_fd = get_socket_fd_for_player(opponent_id);
        std::string return_to_waiting_msg = "RPS|return_to_waiting;";
        send(opponent_fd, return_to_waiting_msg.c_str(), return_to_waiting_msg.size(), 0);
        std::cout << "NOTIFY > Opponent " << opponent_id << " sent to waiting screen due to timeout.\n";
        group.remove_player(opponent_id);
        handle_return_to_lobby(opponent_id);
    }

    std::cout << "DEBUG > Player " << player_id << " has been reset and removed from all game contexts.\n"; 
}

int GameServer::get_current_round_for_player(const std::string &player_id)
{
    // Iterate through all groups to find the player
    for (auto &[group_id, group] : groups)
    {
        for (const Player &player : group.players)
        {
            if (player.player_id == player_id)
            {
                // Player found in this group, return current round number
                return group.logic.get_current_round();
            }
        }
    }

    std::cerr << "ERROR > Player " << player_id << " is not found in any group.\n";
    return -1; // Return -1 or another indicative value if player is not found
}
