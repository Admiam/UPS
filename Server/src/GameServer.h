#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <sys/socket.h>
#include "GameLogic.h"
#include <set>

struct Player
{
    std::string player_id;
    int socket_fd;
    bool is_connected;

    Player(const std::string &id, int socket) : player_id(id), socket_fd(socket), is_connected(true) {}
};

struct Group
{
    std::string group_id;
    std::vector<Player> players;
    std::unordered_map<std::string, std::string> choices; // Player choices (player_id -> choice)
    GameLogic logic;
    std::unordered_map<std::string, bool> ready_state;

    bool all_players_ready() const
    {
        return std::all_of(players.begin(), players.end(),
                           [this](const Player &p)
                           {
                               return ready_state.at(p.player_id);
                           });
    }

    bool is_full() const { return players.size() >= 2; }

    void add_player(const Player &player) { players.push_back(player); }

    void remove_player(const std::string &player_id)
    {
        players.erase(std::remove_if(players.begin(), players.end(),
                                     [&player_id](const Player &p)
                                     { return p.player_id == player_id; }),
                      players.end());
    }

    bool contains_player(const std::string &player_id) const
    {
        return std::any_of(players.begin(), players.end(),
                           [&player_id](const Player &p)
                           { return p.player_id == player_id; });
    }

    // Register a player's choice and check if the round is ready to be processed
    bool register_choice(const std::string &player_id, const std::string &choice)
    {
        choices[player_id] = choice;
        return choices.size() == players.size(); // Round is ready when all players have made their choices
    }

    // Clear choices for the next round
    void reset_choices() { choices.clear(); }
};

struct DisconnectedPlayer
{
    std::string group_id;
    bool is_reconnecting; // True if player is expected to reconnect
    std::chrono::time_point<std::chrono::steady_clock> disconnect_time;
};


class GameServer
{
public:
    GameServer() : next_group_number(1) {}
    void add_player_to_queue(const std::string &player_id, int socket_fd);
    void attempt_to_form_group();
    void handle_disconnect(const std::string &player_id);
    GameLogic game_logic; // Instance of GameLogic
    void process_player_choice(const std::string &player_id, const std::string &choice);
    std::string get_player_group(const std::string &player_id);
    bool register_choice(const std::string &group_id, const std::string &player_id, const std::string &choice);
    void process_group_round(const std::string &group_id);
    void handle_ready_message(const std::string &player_id);
    void handle_return_to_lobby(const std::string &player_id);
    void update_ping(const std::string &player_id);
    void check_for_timeouts();
    void handle_internet_disconnect(const std::string &player_id);
    void cleanup();

private: 
    std::unordered_map<std::string, Group> groups;
    std::unordered_map<std::string, Player *> player_directory;
    std::queue<std::pair<std::string, int>> player_queue; 
    std::unordered_map<std::string, DisconnectedPlayer> disconnected_players;
    std::unordered_map<int, std::string> socket_to_player_id;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> player_last_ping;
    int next_group_number;
    std::mutex game_mutex;

    void create_group(const std::string &group_id);
    void start_reconnection_timer(const std::string &group_id, const std::string &player_id);
    void handle_reconnection_timeout(const std::string &group_id, const std::string &player_id);
    int get_socket_fd_for_player(const std::string &player_id) const;
    void notify_players_to_start(const Group &group);
    void handle_reconnection(const std::string &player_id, int socket_fd);
    void mark_player_ready(const std::string &group_id, const std::string &player_id);
    void print_to_hex(const std::string &str) const;
    std::string normalize_player_id(const std::string &player_id) const;
    void print_groups() const;
    void print_player_queue() const;
    void print_disconnected_players() const;
    void disconnect_player_due_to_timeout(const std::string &player_id);
};
