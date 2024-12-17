#include "GameLogic.h"
#include <iostream>
#include <iomanip>

void GameLogic::process_round(const PlayerMove &player1, const PlayerMove &player2)
{
    // Determine the winner
    // std::cout << "Player 1 choice: " << extract_payload(player1.choice) << "\n";
    // std::cout << "Player 2 choice: " << extract_payload(player2.choice) << "\n";
    std::string result = determine_winner(player1.choice, player2.choice);
    std::string trimmed_choice1 = normalize_string(trim(extract_payload(player1.choice)));

    if (result == "draw")
    {
        // std::cout << "Round is a draw!\n";
    }
    else if (result == trimmed_choice1)
    {
        // std::cout << "Player 1 wins the round!\n";
        player_scores[player1.player_id]++; // Increment winner's score
    }
    else
    {
        // std::cout << "Player 2 wins the round!\n";
        player_scores[player2.player_id]++; // Increment winner's score
    }
}

int GameLogic::get_score(const std::string &player_id) const
{
    auto it = player_scores.find(player_id);
    return it != player_scores.end() ? it->second : 0;
}

std::string GameLogic::determine_winner(const std::string &choice1, const std::string &choice2)
{
    std::string trimmed_choice1 = normalize_string(trim(extract_payload(choice1)));
    std::string trimmed_choice2 = normalize_string(trim(extract_payload(choice2)));

    if (trimmed_choice1 == trimmed_choice2)
    {
        return "draw";
    }

    // Rock beats Scissors, Scissors beats Paper, Paper beats Rock
    if ((trimmed_choice1 == "rock" && trimmed_choice2 == "scissors") ||
        (trimmed_choice1 == "scissors" && trimmed_choice2 == "paper") ||
        (trimmed_choice1 == "paper" && trimmed_choice2 == "rock"))
    {
        return trimmed_choice1; // Player 1's choice wins
    }

    return trimmed_choice2; // Player 2's choice wins
}

std::string GameLogic::extract_payload(const std::string &message)
{
    if (message.size() <= 4)
    {
        return ""; // Invalid message, return an empty string
    }
    return message.substr(4); // Skip the first 4 bytes
}

void GameLogic::print_hex(const std::string &str)
{
    for (unsigned char c : str)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << "\n";
}

std::string GameLogic::normalize_string(const std::string &str)
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

std::string GameLogic::trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return ""; // String is all whitespace

    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}