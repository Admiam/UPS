#include <vector>
#include <string>
#include <sstream>

// Function to split a string by a delimiter (in this case, '|')
// std::vector<std::string> split(const std::string &s, char delimiter)
// {
//     std::vector<std::string> tokens;
//     std::string token;
//     std::istringstream tokenStream(s);
//     while (std::getline(tokenStream, token, delimiter))
//     {
//         tokens.push_back(token);
//     }
//     return tokens;
// }

// std::string extract_payload(const std::string &message)
// {
//     if (message.size() <= 4)
//     {
//         return ""; // Invalid message, return an empty string
//     }
//     return message.substr(4); // Skip the first 4 bytes
// }

// std::string normalize_string(const std::string &str)
// {
//     std::string normalized;
//     for (size_t i = 0; i < str.size(); ++i)
//     {
//         // Check if the current and next byte represent \u00A0 in UTF-8
//         if (i + 1 < str.size() && static_cast<unsigned char>(str[i]) == 0xC2 &&
//             static_cast<unsigned char>(str[i + 1]) == 0xA0)
//         {
//             normalized += ' '; // Replace non-breaking space with a regular space
//             ++i;               // Skip the next byte
//         }
//         else
//         {
//             normalized += str[i];
//         }
//     }
//     return normalized;
// }

// std::string trim(const std::string &str)
// {
//     size_t first = str.find_first_not_of(" \t\n\r");
//     if (first == std::string::npos)
//         return ""; // String is all whitespace

//     size_t last = str.find_last_not_of(" \t\n\r");
//     return str.substr(first, (last - first + 1));
// }
