#include "TCPServer.h"
#include <iostream>

// Constructor: Initialize the server on the specified port
TCPServer::TCPServer(int port) : port(port), server_socket(-1)
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
// Destructor: Close the server socket if it's open
TCPServer::~TCPServer()
{
    if (server_socket != -1)
        close(server_socket);
}

// Configure and bind the server socket
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
    my_addr.sin_family = AF_INET;         // Use IPv4
    my_addr.sin_port = htons(port);       // Specify the port (network byte order)
    my_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address

    // Bind the socket to the specified port and address
    if (bind(server_socket, reinterpret_cast<sockaddr *>(&my_addr), sizeof(my_addr)) == -1)
    {
        throw std::runtime_error("Bind error");
    }
    std::cout << "Bind - OK\n";

    // Set the socket to listen mode with a backlog of 5 connections
    if (listen(server_socket, 5) == -1)
    {
        throw std::runtime_error("Listen error");
    }
    std::cout << "Listen - OK\n";

    // Add the server socket to the client sockets set
    FD_SET(server_socket, &client_socks);
}

// Main loop to handle new connections and data from clients
void TCPServer::run()
{
    fd_set tests; // Temporary file descriptor set for select()
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
        if (activity < 0)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted by a signal, continue
            }
            std::cerr << "Select error\n";
            break;
        }

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

// Accept and add a new client to the client sockets set
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

// Example usage in handleClientData
void TCPServer::handleClientData(int fd)
{

    int a2read = 0;
    if (ioctl(fd, FIONREAD, &a2read) < 0)
    {
        std::cerr << "ERROR > ioctl failed on fd " << fd << ": " << strerror(errno) << "\n";
        handleClientDisconnection(fd);
        return;
    }

    if (a2read > 0)
    {
        std::vector<char> buffer(a2read);
        int bytes_received = recv(fd, buffer.data(), a2read, 0);

        if (bytes_received > 0)
        {
            std::string message(buffer.begin(), buffer.begin() + bytes_received);
            std::cout << "CLIENT > " << message << std::endl;
            std::vector<std::string> parts = split(message, '|');


            if (message == "RPS|ping")
            {
                std::string player_id = get_player_id_from_socket(fd);
                if (player_id.empty())
                {
                    std::cerr << "ERROR > Unknown player sending ping from socket " << fd << ".\n";
                    return;
                }
                game_server.update_ping(player_id);
                // Respond with a pong
                std::string pong_message = "RPS|pong;";
                std::cout << "SERVER > send: " << pong_message << "\n";

                send(fd, pong_message.c_str(), pong_message.size(), 0);
                if (game_server.is_player_reconnecting(player_id))
                {
                    game_server.handle_reconnection(player_id, fd);
                    game_server.notify_opponent_reconnected(player_id);
                }
                game_server.check_for_inactive_players();
            }
            else if (parts.size() > 1 && parts[1] == "login")
                {
                    std::string player_id = parts[2];
                    game_server.add_player_to_queue(player_id, fd);
                    socket_to_player_id[fd] = player_id; // Map socket FD to player ID
                }
            else if (parts.size() > 1 && parts[1] == "lobby")
            {
                if (parts.size() == 3)
                {
                    std::string player_id = parts[2];

                    game_server.add_player_to_queue(player_id, fd);
                    socket_to_player_id[fd] = player_id; // Map socket FD to player ID
                }
                else
                {
                    std::cerr << "Invalid 'lobby' message format.\n";
                    std::string error_msg = "error|invalid_message_format";
                    send(fd, error_msg.c_str(), error_msg.size(), 0);
                    // TODO disconnect client
                }
            }
            else if (parts.size() > 1 && parts[1] == "ready")
                {
                    if (parts.size() < 3)
                    {
                        std::cerr << "Invalid 'ready' message format: " << message << "\n";
                        return;
                    }

                    std::string player_id = parts[2];

                    std::cout << "Player " << player_id << " is ready.\n";

                    // Notify the GameServer about the readiness of the player
                    game_server.handle_ready_message(player_id);
                }
            else if (parts.size() > 2 && parts[1] == "game")
            {
                std::string choice = parts[2];
                std::string player_id = socket_to_player_id[fd];
                std::string group_id = game_server.get_player_group(player_id);

                if (group_id.empty())
                {
                    std::cerr << "Player " << player_id << " is not part of any group.\n";
                    return;
                }

                if (game_server.register_choice(group_id, player_id, choice))
                {
                    game_server.process_group_round(group_id);
                }
            }
            else if (parts.size() > 1 && parts[1] == "return_to_lobby")
            {
                if (parts.size() < 3)
                {
                    std::cerr << "Invalid 'return_to_lobby' message format.\n";
                    return;
                }

                std::string player_id = socket_to_player_id[fd];
                ;

                std::cout << "Player " << player_id << " is returning to the lobby.\n";

                // Notify the GameServer to handle the return to lobby
                game_server.handle_return_to_lobby(player_id);

                // Send confirmation to the client
                std::string confirmation_msg = "RPS|lobby|success;";
                send(fd, confirmation_msg.c_str(), confirmation_msg.size(), 0);
            }
            else if (parts.size() > 1 && parts[1] == "reconnect")
            {
                if (parts.size() < 3)
                {
                    std::cerr << "Invalid 'reconnect' message format.\n";
                    return;
                }

                std::string player_id = parts[2];
                std::string group_id = game_server.get_player_group(player_id);

                if (group_id.empty())
                {
                    std::cerr << "Reconnection failed: Player " << player_id << " is not part of any group.\n";
                    return;
                }

                // Update the socket mapping for the player
                socket_to_player_id[fd] = player_id;

                // Notify the GameServer about the reconnection
                game_server.handle_reconnection(player_id, fd);

                // Respond to the client
                // std::string success_msg = "RPS|opponent_reconnected;";
                // send(fd, success_msg.c_str(), success_msg.size(), 0);
                game_server.notify_opponent_reconnected(player_id);
                std::cout << "RECONNECT > player " << player_id << " with new socket FD: " << fd << "\n";
            }
        }
        else
        {
            std::cerr << "recv failed on fd " << fd << ": " << strerror(errno) << "\n";
            handleClientDisconnection(fd);
        }
    }
    else
    { 
        handleClientDisconnection(fd);
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

int main()
{
    try
    {
        TCPServer server(4242); // Create the server to listen on port 4242
        server.run();           // Start the server
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
