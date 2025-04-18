#include "bitmap_index/utils/string_util.h"
#include <sstream>      // For std::stringstream in split
#include <algorithm>    // For std::find_if_not
#include <cctype>       // For std::isspace

namespace bitmap_index::utils {

std::vector<std::string> split(std::string_view s, char delimiter, bool skip_empty) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != std::string_view::npos) {
        std::string_view token = s.substr(start, end - start);
        if (!skip_empty || !token.empty()) {
            result.emplace_back(token); // Construct string from view
        }
        start = end + 1;
        end = s.find(delimiter, start);
    }

    // Add the last token (after the last delimiter or the only token if no delimiter)
    std::string_view last_token = s.substr(start);
    if (!skip_empty || !last_token.empty()) {
        result.emplace_back(last_token);
    }

    return result;
}


// Helper lambda for checking if a character is whitespace
auto is_whitespace = [](const unsigned char c){ return std::isspace(c); };

std::string& trimLeft(std::string& s) {
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), is_whitespace));
    return s;
}

std::string& trimRight(std::string& s) {
    s.erase(std::find_if_not(s.rbegin(), s.rend(), is_whitespace).base(), s.end());
    return s;
}

std::string& trim(std::string& s) {
    return trimLeft(trimRight(s));
}

std::string trimLeftCopy(std::string_view s) {
    const auto it = std::find_if_not(s.begin(), s.end(), is_whitespace);
    return {it, s.end()}; // Construct string from iterators
}

std::string trimRightCopy(std::string_view s)
{
    const auto it = std::find_if_not(s.rbegin(), s.rend(), is_whitespace);
    // it.base() points one element *past* the non-whitespace char in forward direction
    return {s.begin(), it.base()
};
}

std::string trimCopy(std::string_view s)
{
    const auto first = std::find_if_not(s.begin(), s.end(), is_whitespace);
    const auto last = std::find_if_not(s.rbegin(), s.rend(), is_whitespace).base();
    // Handle case where the string is all whitespace
    if (first >= last) {
        return "";
    }
    return {first, last}; // Construct from valid range
}

} // namespace bitmap_index::utils