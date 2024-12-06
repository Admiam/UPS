#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <string>
#include <unordered_map>
#include <iostream>
#include <stdexcept>

struct PlayerMove
{
    std::string player_id;
    std::string choice; // "rock", "paper", or "scissors"
};

class GameLogic
{
public:
    // Update scores based on player choices
    void process_round(const PlayerMove &player1, const PlayerMove &player2);

    // Get the current score of a player
    int get_score(const std::string &player_id) const;
    GameLogic() : current_round(1) {}

    int get_current_round() const { return current_round; }
    void increment_round() { current_round++; }
    void reset_round() { current_round = 1; }

private:
    int current_round; // Tracks the current round number

    std::unordered_map<std::string, int> player_scores; // Map player IDs to their scores
    std::string determine_winner(const std::string &choice1, const std::string &choice2);
    std::string trim(const std::string &str);
    std::string normalize_string(const std::string &str);
    void print_hex(const std::string &str);
    std::string extract_payload(const std::string &message);
};

#endif // GAME_LOGIC_H
