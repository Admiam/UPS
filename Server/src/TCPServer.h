#ifndef TCPServer_H
#define TCPServer_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
// #include "utils.cpp"
#include "GameServer.h" // Include GameServer

// TCPServer class represents a simple TCP server
class TCPServer
{
public:
    TCPServer(int port); // Constructor initializes the server with a given port
    ~TCPServer();        // Destructor closes the server socket
    void run();          // Main loop to handle incoming connections and data

private:
    int server_socket;   // File descriptor for the server socket
    fd_set client_socks; // Set of file descriptors for the client sockets
    int port;            // Port number for the server
    GameServer game_server; // Declare GameServer instance
    std::unordered_map<int, std::string> socket_to_player_id;

    void setupSocket();            // Set up and configure the server socket
    void handleNewConnection();    // Accept and manage a new client connection
    void handleClientData(int fd); // Receive and process data from a client socket
    void handleClientDisconnection(int fd);
    std::string get_player_id_from_socket(int fd);
    std::vector<std::string> split(const std::string &s, char delimiter);
    void cleanup();

};
#endif