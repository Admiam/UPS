#include "TCPServer.h"
#include "utils.cpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

TCPServer::TCPServer(const std::string &ip, int port) : ip(ip), port(port), server_socket(-1)
{
    FD_ZERO(&client_socks); // Clear the client sockets set
    setupSocket();          // Set up the server socket
}

std::string TCPServer::get_player_id_from_socket(int fd)
{
    auto it = socket_to_player_id.find(fd);
    if (it != socket_to_player_id.end())
    {
        return it->second; // Return player ID if found
    }
    return ""; // Return an empty string if player ID is not found
}

TCPServer::~TCPServer()
{
    if (server_socket != -1)
        close(server_socket);
}

void TCPServer::setupSocket()
{
    // Create a TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options to allow reuse of the address
    int param = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &param, sizeof(param)) == -1)
    {
        throw std::runtime_error("setsockopt error");
    }

    // Set up the server address structure
    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;   // Use IPv4
    my_addr.sin_port = htons(port); // Specify the port (network byte order)

    // Convert the IP address string to binary form
    if (inet_pton(AF_INET, ip.c_str(), &my_addr.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid IP address: " + ip);
    }

    // Bind the socket to the specified port and address
    if (bind(server_socket, reinterpret_cast<sockaddr *>(&my_addr), sizeof(my_addr)) == -1)
    {
        throw std::runtime_error("Bind error: " + std::string(strerror(errno)));
    }
    std::cout << "Bind - OK to IP " << ip << ", Port " << port << "\n";

    // Set the socket to listen mode with a backlog of 5 connections
    if (listen(server_socket, 5) == -1)
    {
        throw std::runtime_error("Listen error: " + std::string(strerror(errno)));
    }
    std::cout << "Listen - OK\n";

    // Add the server socket to the client sockets set
    FD_SET(server_socket, &client_socks);
}

void TCPServer::run()
{
    fd_set tests;           // Temporary file descriptor set for select()
    struct timeval timeout; // Timeout for select()
    auto last_check = std::chrono::steady_clock::now();
    bool running = true; // Server loop flag

    while (running)
    {
        tests = client_socks; // Copy the client sockets set (select modifies it)

        timeout.tv_sec = 1;  // Check for activity every 1 second
        timeout.tv_usec = 0; // No microseconds

        // Wait for activity on any socket (blocking)
        int activity = select(FD_SETSIZE, &tests, nullptr, nullptr, nullptr);
        // if (activity < 0)
        // {
        //     if (errno == EINTR)
        //     {
        //         continue; // Interrupted by a signal, continue
        //     }
        //     std::cerr << "Select error\n";
        //     break;
        // }

        // Loop through file descriptors to check which one has activity
        for (int fd = 3; fd < FD_SETSIZE; ++fd)
        {
            if (FD_ISSET(fd, &tests)) // Check if fd is set in the tests set
            {
                if (fd == server_socket)
                {
                    try
                    {
                        handleNewConnection();
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error handling new connection: " << e.what() << "\n";
                    }
                }
                else
                {
                    try
                    {
                        handleClientData(fd);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error handling client data: " << e.what() << "\n";
                    }
                }
            }
        }
        // Periodically check for timeouts
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() >= 3)
        {
            try
            {
                game_server.check_for_timeouts();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error during timeout check: " << e.what() << "\n";
            }
            last_check = now;
        }
    }
    cleanup();
}

void TCPServer::handleNewConnection()
{
    sockaddr_in peer_addr{}; // Client address structure
    socklen_t len_addr = sizeof(peer_addr);

    // Accept a new connection from the client
    int client_socket = accept(server_socket, reinterpret_cast<sockaddr *>(&peer_addr), &len_addr);
    if (client_socket == -1)
    {
        std::cerr << "Accept error\n";
    }
    else
    {
        FD_SET(client_socket, &client_socks); // Add new client socket to the set
        std::cout << "New client connected and added to socket set:" << client_socket << "\n";
    }
}

void TCPServer::handleClientData(int fd)
{

    int a2read = 0;
    if (ioctl(fd, FIONREAD, &a2read) < 0)
    {
        std::cerr << "ERROR > ioctl failed on fd " << fd << ": " << strerror(errno) << "\n";
        // std::string player_id = get_player_id_from_socket(fd);
        std::string fd_player = get_player_id_from_socket(fd);

        if (!fd_player.empty())
        {
            game_server.reset_and_remove_player(fd_player);
        }
        std::string error_message = "RPS|error|Player not found;";
        send(fd, error_message.c_str(), error_message.size(), 0);
        close(fd); // Disconnect the client
        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
    }

        if (a2read > 0)
        {
            std::vector<char> buffer(a2read);
            int bytes_received = recv(fd, buffer.data(), a2read, 0);

            if (bytes_received > 0)
            {
                std::string message(buffer.begin(), buffer.begin() + bytes_received);
                std::vector<std::string> parts = split(message, '|');

                if (parts.empty() || parts[0] != "RPS")
                {
                    std::cerr << "Invalid message format from client on socket " << fd << ".\n";

                    // Notify GameServer to handle disconnection and cleanup
                    // std::string player_id = get_player_id_from_socket(fd);
                    std::string fd_player = get_player_id_from_socket(fd);

                    if (!fd_player.empty())
                    {
                        game_server.reset_and_remove_player(fd_player);
                    }
                    std::string error_message = "RPS|error|Player not found;";
                    send(fd, error_message.c_str(), error_message.size(), 0);
                    close(fd);                 // Close the socket
                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    // handleClientDisconnection(fd);
                    // return; // Exit after handling invalid message
                }
                else
                {
                    std::cout << "CLIENT > " << message << std::endl;
                }

                if (message == "RPS|ping")
                {
                    std::string player_id = get_player_id_from_socket(fd);
                    if (player_id.empty())
                    {
                        std::cerr << "ERROR > Unknown player sending ping from socket " << fd << ".\n";
                        std::string error_message = "RPS|error|Unknown player;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd); // Disconnect the unidentified client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        game_server.update_ping(player_id);
                        std::string pong_message = "RPS|pong;";
                        send(fd, pong_message.c_str(), pong_message.size(), 0);
                        if (game_server.is_player_reconnecting(player_id))
                        {
                            game_server.handle_reconnection(player_id, fd);
                            game_server.notify_opponent_reconnected(player_id);
                        }
                        game_server.check_for_inactive_players();
                    }
                }
                else if (parts.size() == 3 && parts[1] == "login")
                {
                    // Check if parts[2] exists
                    if (parts[2].empty())
                    {
                        std::cerr << "ERROR > Login message is missing player ID.\n";
                        std::string error_message = "RPS|error|Missing player ID;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd); // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        // Normalize and validate player ID
                        std::string player_id = game_server.normalize_string(game_server.trim(game_server.extract_payload(parts[2])));
                        if (player_id.empty())
                        {
                            std::cerr << "ERROR > Invalid or empty player ID provided in login message.\n";
                            std::string error_message = "RPS|error|Invalid player ID;";
                            send(fd, error_message.c_str(), error_message.size(), 0);
                            std::string fd_player = get_player_id_from_socket(fd);
                            game_server.reset_and_remove_player(fd_player);
                            close(fd); // Disconnect the client
                            FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set

                        }
                        else
                        {
                            std::string fd_player = get_player_id_from_socket(fd);
                            Player *player = game_server.get_player_by_id(fd_player);
                            if (player)
                            {
                                // Check if the player is already in a game or reconnecting
                                if (player->state == PLAYING || player->state == RECONNECTING || player->state == WAITING || player->state == LOBBY || player_id != fd_player)
                                {
                                    std::cerr << "ERROR > Player " << fd_player << " is already in a game or reconnecting.\n";

                                    // Notify the player about the invalid action and disconnect them
                                    std::string error_message = "RPS|error|Invalid login attempt;";
                                    send(fd, error_message.c_str(), error_message.size(), 0);
                                    // game_server.remove_player_from_queue(player_id);
                                    game_server.reset_and_remove_player(fd_player);
                                    // game_server.handle_invalid_message(player_id);
                                    close(fd);                 // Disconnect the client
                                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                                }
                            }
                            else
                            {
                                // Proceed with login logic
                                game_server.update_player_state(player_id, LOBBY); // Set the player's state to LOBBY
                                game_server.add_player_to_queue(player_id, fd);
                                socket_to_player_id[fd] = player_id;               // Map socket FD to player ID
                                std::cout << "LOBBY > Player " << player_id << " logged in successfully.\n";
                            }
                        }
                    }
                }
                else if (parts.size() == 3 && parts[1] == "ready")
                {
                    // Retrieve the player_id using the socket FD
                    auto it = socket_to_player_id.find(fd);
                    if (it == socket_to_player_id.end())
                    {
                        std::cerr << "ERROR > Socket FD " << fd << " is not associated with any player.\n";
                        std::string error_message = "RPS|error|Invalid socket association;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd); // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        // Check if parts[2] exists
                        if (parts[2].empty())
                        {
                            std::cerr << "ERROR > Login message is missing player ID.\n";
                            std::string error_message = "RPS|error|Missing player ID;";
                            send(fd, error_message.c_str(), error_message.size(), 0);
                            std::string player_id = get_player_id_from_socket(fd);
                            if (!player_id.empty())
                            {
                                game_server.reset_and_remove_player(player_id);
                            }
                            close(fd); // Disconnect the client
                            FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                        }
                        else
                        {
                            // Normalize and validate player ID
                            std::string player_id = game_server.normalize_string(game_server.trim(game_server.extract_payload(parts[2])));
                            if (player_id.empty())
                            {
                                std::cerr << "ERROR > Invalid or empty player ID provided in login message.\n";
                                std::string error_message = "RPS|error|Invalid player ID;";
                                std::string fd_player = get_player_id_from_socket(fd);
                                game_server.reset_and_remove_player(fd_player);
                                send(fd, error_message.c_str(), error_message.size(), 0);
                                close(fd); // Disconnect the client
                                FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                            }
                            else
                            {
                                // Retrieve the Player object
                                std::string fd_player = get_player_id_from_socket(fd);
                                Player *player = game_server.get_player_by_id(fd_player);
                                if (!player)
                                {
                                    std::cerr << "ERROR > Player ID " << player_id << " does not exist.\n";
                                    std::string error_message = "RPS|error|Player not found;";
                                    send(fd, error_message.c_str(), error_message.size(), 0);
                                    std::string player_id = get_player_id_from_socket(fd);
                                    if (!player_id.empty())
                                    {
                                        game_server.reset_and_remove_player(player_id);
                                    }
                                    close(fd); // Disconnect the client
                                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                                }
                                else if (player->state == LOBBY && player_id == fd_player)
                                {
                                    game_server.handle_ready_message(player_id);
                                    game_server.update_player_state(player_id, PLAYING); // Set the player's state to LOBBY
                                    std::cout << "INFO > Player " << player_id << " is ready.\n";
                                }
                                else
                                {
                                    std::cerr << "ERROR > Player " << fd_player << " attempted invalid ready action while in state " << player->state << ".\n";
                                    std::string error_message = "RPS|error|Invalid action;";
                                    send(fd, error_message.c_str(), error_message.size(), 0);
                                    game_server.reset_and_remove_player(fd_player);
                                    close(fd); // Disconnect the client
                                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set

                                    // game_server.handle_invalid_message(player_id);
                                }
                            }
                        }
                    }
                }
                else if (parts.size() == 3 && parts[1] == "game")
                {
                    // Retrieve the player_id using the socket FD
                    auto it = socket_to_player_id.find(fd);
                    if (it == socket_to_player_id.end())
                    {
                        std::cerr << "ERROR > Socket FD " << fd << " is not associated with any player.\n";
                        std::string error_message = "RPS|error|Invalid socket association;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd); // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        std::string player_id = it->second;

                        // Retrieve the Player object
                        std::string fd_player = get_player_id_from_socket(fd);
                        Player *player = game_server.get_player_by_id(fd_player);
                        if (!player)
                        {
                            std::cout << "ERROR > Player ID " << player_id << " does not exist.\n";
                            std::string error_message = "RPS|error|Player not found;";
                            send(fd, error_message.c_str(), error_message.size(), 0);
                            close(fd); // Disconnect the client
                            FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                        }
                        else
                        {
                            try
                            {
                                std::string choice = game_server.normalize_string(game_server.trim(game_server.extract_payload(parts[2])));

                                if (player->state == PLAYING && (choice == "rock" || choice == "paper" || choice == "scissors") && player_id == fd_player)
                                {
                                    choice = parts[2];
                                    std::string player_id = socket_to_player_id[fd];
                                    std::string group_id = game_server.get_player_group(player_id);

                                    if (game_server.register_choice(group_id, player_id, choice))
                                    {
                                        game_server.process_group_round(group_id);
                                    }
                                }
                                else
                                {
                                    std::cerr << "ERROR > Player " << fd_player << " attempted invalid game action while in state " << player->state << ".\n";
                                    std::string error_message = "RPS|error|Invalid action;";
                                    send(fd, error_message.c_str(), error_message.size(), 0);
                                    game_server.reset_and_remove_player(fd_player);
                                    close(fd); // Disconnect the client
                                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                                }
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "ERROR > Exception caught: " << e.what() << "\n";
                                std::string error_message = "RPS|error|Invalid action;";
                                send(fd, error_message.c_str(), error_message.size(), 0);
                                game_server.reset_and_remove_player(fd_player);
                                close(fd); // Disconnect the client
                                FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                            }
                        }
                    }
                }
                else if (parts.size() == 3 && parts[1] == "return_to_lobby")
                {
                    // Retrieve the player_id using the socket FD
                    auto it = socket_to_player_id.find(fd);
                    if (it == socket_to_player_id.end())
                    {
                        std::cerr << "ERROR > Socket FD " << fd << " is not associated with any player.\n";
                        std::string error_message = "RPS|error|Invalid socket association;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd); // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        // Normalize and validate player ID
                        std::string player_id = game_server.normalize_string(game_server.trim(game_server.extract_payload(parts[2])));
                        if (player_id.empty())
                        {
                            std::cerr << "ERROR > Invalid or empty player ID provided in login message.\n";
                            std::string error_message = "RPS|error|Invalid player ID;";
                            send(fd, error_message.c_str(), error_message.size(), 0);
                            close(fd); // Disconnect the client
                            FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                        }
                        else
                        {
                            std::string fd_player = get_player_id_from_socket(fd);
                            Player *player = game_server.get_player_by_id(fd_player);
                            std::cout << "Player ID: " << player_id << " Player FD: " << fd_player << " current round: " << game_server.get_current_round_for_player(player_id) << "\n";
                            if (player->state == PLAYING && player_id == fd_player && game_server.get_current_round_for_player(player_id) == 11)
                            {
                                game_server.handle_return_to_lobby(player_id);
                                game_server.update_player_state(player_id, LOBBY); // Set the player's state to LOBBY
                                // Send confirmation to the client
                                std::string confirmation_msg = "RPS|return_to_waiting;";
                                send(fd, confirmation_msg.c_str(), confirmation_msg.size(), 0);
                                std::cout << "SERVER > send: " << confirmation_msg << "\n";
                            }
                            else
                            {
                                std::cerr << "ERROR > Player " << fd_player << " attempted invalid return to lobby action while in state " << player->state << ".\n";
                                std::string error_message = "RPS|error|Invalid action;";
                                send(fd, error_message.c_str(), error_message.size(), 0);
                                game_server.reset_and_remove_player(fd_player);
                                close(fd); // Disconnect the client
                                FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                            }
                        }
                    }
                }
                else if (parts.size() == 3 && parts[1] == "reconnect")
                {
                    // Retrieve the player_id using the socket FD
                    // auto it = socket_to_player_id.find(fd);
                    // if (it == socket_to_player_id.end())
                    // {
                    //     std::cerr << "ERROR > Socket FD " << fd << " is not associated with any player.\n";
                    //     std::string error_message = "RPS|error|Invalid socket association;";
                    //     send(fd, error_message.c_str(), error_message.size(), 0);
                    //     close(fd); // Disconnect the client
                    //     FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    // }
                    // else 
                    if (parts[2].empty())
                    {
                        std::cerr << "ERROR > Login message is missing player ID.\n";
                        std::string error_message = "RPS|error|Missing player ID;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        std::string player_id = get_player_id_from_socket(fd);
                        if (!player_id.empty())
                        {
                            game_server.reset_and_remove_player(player_id);
                        }
                        close(fd); // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        // Normalize and validate player ID
                        std::string player_id = game_server.normalize_string(game_server.trim(game_server.extract_payload(parts[2])));
                        if (player_id.empty())
                        {
                            std::cerr << "ERROR > Invalid or empty player ID provided in login message.\n";
                            std::string error_message = "RPS|error|Invalid player ID;";
                            send(fd, error_message.c_str(), error_message.size(), 0);
                            close(fd); // Disconnect the client
                            FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                        }
                        else
                        {
                            // Retrieve the Player object
                            // std::string fd_player = get_player_id_from_socket(fd);
                            Player *player = game_server.get_player_by_id(player_id);
                            if (!player)
                            {
                                std::cerr << "ERROR > Player ID " << player_id << " does not exist.\n";
                                std::string error_message = "RPS|error|Player not found;";
                                send(fd, error_message.c_str(), error_message.size(), 0);
                                std::string player_id = get_player_id_from_socket(fd);
                                if (!player_id.empty())
                                {
                                    game_server.reset_and_remove_player(player_id);
                                }
                                close(fd); // Disconnect the client
                                FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                            }
                            else if (player->state == RECONNECTING && player_id == player_id)
                            {
                                std::string group_id = game_server.get_player_group(player_id);
                                // Update the socket mapping for the player
                                socket_to_player_id[fd] = player_id;

                                // Notify the GameServer about the reconnection
                                game_server.handle_reconnection(player_id, fd);

                                // Respond to the client
                                // std::string success_msg = "RPS|opponent_reconnected;";
                                // send(fd, success_msg.c_str(), success_msg.size(), 0);
                                game_server.notify_opponent_reconnected(player_id);
                                game_server.update_player_state(player_id, PLAYING); // Set the player's state to LOBBY
                                std::cout << "RECONNECT > player " << player_id << " with new socket FD: " << fd << "\n";
                            }
                            else
                            {
                                std::cerr << "ERROR > Player " << player_id << " attempted invalid reconnection action while in state " << player->state << ".\n";
                                std::string error_message = "RPS|error|Invalid action;";
                                send(fd, error_message.c_str(), error_message.size(), 0);
                                game_server.reset_and_remove_player(player_id);
                                close(fd); // Disconnect the client
                                FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                            }
                        }
                    }
                }
                else
                {
                    std::string player_id = get_player_id_from_socket(fd);
                    if (player_id.empty())
                    {
                        std::cerr << "ERROR > Socket FD " << fd << " invalid message.\n";
                        std::string error_message = "RPS|error|Invalid socket association;";
                        socket_to_player_id[fd] = ""; // Map socket FD to player ID
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        close(fd);                 // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                    else
                    {
                        // std::string player_id = it->second;
                        std::string error_message = "RPS|error|Invalid socket association;";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                        game_server.reset_and_remove_player(player_id);
                        close(fd);                 // Disconnect the client
                        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                    }
                }
            }
            else
            {
                std::cerr << "recv failed on fd " << fd << ": " << strerror(errno) << "\n";
                // handleClientDisconnection(fd);
                auto it = socket_to_player_id.find(fd);
                if (it == socket_to_player_id.end())
                {
                    std::cerr << "ERROR > Socket FD " << fd << " is not associated with any player.\n";
                    std::string error_message = "RPS|error|Invalid socket association;";
                    send(fd, error_message.c_str(), error_message.size(), 0);
                    close(fd); // Disconnect the client
                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                }
                else
                {
                    std::string player_id = it->second;
                    game_server.reset_and_remove_player(player_id);
                    close(fd); // Disconnect the client
                    FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set
                }
            }
    }
    else
    {
        std::string fd_player = get_player_id_from_socket(fd);

        if (!fd_player.empty())
        {
            game_server.reset_and_remove_player(fd_player);
        }
        std::string error_message = "RPS|error|Player not found;";
        send(fd, error_message.c_str(), error_message.size(), 0);
        close(fd);                 // Disconnect the client
        FD_CLR(fd, &client_socks); // IMPORTANT: Remove the socket from the set    
    }
}

void TCPServer::handleClientDisconnection(int fd)
{
    std::string player_id = get_player_id_from_socket(fd);
    if (!player_id.empty())
    {
        game_server.handle_disconnect(player_id);
        socket_to_player_id.erase(fd); // Remove entry from map
    }
    close(fd);
    FD_CLR(fd, &client_socks);
}

std::vector<std::string> TCPServer::split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void TCPServer::cleanup()
{
    std::cout << "Cleaning up server resources...\n";

    // Close all active client sockets
    for (int fd = 0; fd < FD_SETSIZE; ++fd)
    {
        if (FD_ISSET(fd, &client_socks)) // Check if this socket is active
        {
            close(fd);                 // Close the socket
            FD_CLR(fd, &client_socks); // Remove from the file descriptor set
            std::cout << "Closed client socket: " << fd << "\n";
        }
    }

    // Close the server socket
    if (server_socket != -1)
    {
        close(server_socket);
        server_socket = -1;
        std::cout << "Closed server socket.\n";
    }

    // Clear any remaining resources in the game server
    try
    {
        game_server.cleanup(); // Call cleanup on the game server if it has a similar method
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during game server cleanup: " << e.what() << "\n";
    }

    std::cout << "Server cleanup completed. Exiting...\n";
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP> <Port>\n";
        return EXIT_FAILURE;
    }

    std::string ip = argv[1];
    int port;

    try
    {
        port = std::stoi(argv[2]); // Convert port to an integer
    }
    catch (const std::invalid_argument &)
    {
        std::cerr << "Invalid port number.\n";
        return EXIT_FAILURE;
    }

    try
    {
        TCPServer server(ip, port); // Create the server with IP and port
        server.run();               // Start the server
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
