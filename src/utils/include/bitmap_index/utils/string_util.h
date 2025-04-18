#ifndef BITMAP_INDEX_UTILS_STRING_UTIL_H_
#define BITMAP_INDEX_UTILS_STRING_UTIL_H_

#include <string>
#include <vector>
#include <string_view> // Use string_view for efficiency where possible

namespace bitmap_index {
namespace utils {

/**
 * @brief Splits a string into a vector of substrings based on a delimiter.
 *
 * Example: split("a|b|c", '|') -> {"a", "b", "c"}
 * Example: split("a||c", '|') -> {"a", "", "c"}
 * Example: split("|b|c", '|') -> {"", "b", "c"}
 * Example: split("a|b|", '|') -> {"a", "b", ""}
 *
 * @param s The string to split (passed as string_view for efficiency).
 * @param delimiter The character to split by.
 * @param skip_empty If true, empty strings resulting from adjacent delimiters
 *                   or leading/trailing delimiters will be omitted.
 * @return A vector of strings.
 */
std::vector<std::string> split(std::string_view s, char delimiter, bool skip_empty = false);

/**
 * @brief Removes leading whitespace characters from a string (modifies in place).
 * Whitespace is defined by std::isspace.
 * @param s The string to trim.
 * @return Reference to the modified string s.
 */
std::string& trimLeft(std::string& s);

/**
 * @brief Removes trailing whitespace characters from a string (modifies in place).
 * Whitespace is defined by std::isspace.
 * @param s The string to trim.
 * @return Reference to the modified string s.
 */
std::string& trimRight(std::string& s);

/**
 * @brief Removes leading and trailing whitespace characters from a string (modifies in place).
 * Whitespace is defined by std::isspace.
 * @param s The string to trim.
 * @return Reference to the modified string s.
 */
std::string& trim(std::string& s);

/**
 * @brief Removes leading whitespace characters from a string (returns a new string).
 * Whitespace is defined by std::isspace.
 * @param s The input string (passed as string_view).
 * @return A new string with leading whitespace removed.
 */
std::string trimLeftCopy(std::string_view s);

/**
 * @brief Removes trailing whitespace characters from a string (returns a new string).
 * Whitespace is defined by std::isspace.
 * @param s The input string (passed as string_view).
 * @return A new string with trailing whitespace removed.
 */
std::string trimRightCopy(std::string_view s);

/**
 * @brief Removes leading and trailing whitespace characters from a string (returns a new string).
 * Whitespace is defined by std::isspace.
 * @param s The input string (passed as string_view).
 * @return A new string with leading and trailing whitespace removed.
 */
std::string trimCopy(std::string_view s);

} // namespace utils
} // namespace bitmap_index

#endif // BITMAP_INDEX_UTILS_STRING_UTIL_H_