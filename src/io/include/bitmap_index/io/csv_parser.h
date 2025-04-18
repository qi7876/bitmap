#ifndef BITMAP_INDEX_IO_CSV_PARSER_H_
#define BITMAP_INDEX_IO_CSV_PARSER_H_

#include "bitmap_index/core/types.h" // StringId, StringTagSet, FileOffset
#include <string>
#include <vector>
#include <functional>   // For std::function (callback)
#include <filesystem>   // For std::filesystem::path (C++17)
#include <iosfwd>       // For std::istream forward declaration

namespace bitmap_index {
namespace io {

/**
 * @brief Parses CSV files with the format "id | tag1 | tag2 | ..."
 *
 * Handles trimming whitespace around the ID and tags.
 * Uses a callback mechanism to process parsed rows.
 */
class CsvParser {
public:
    /**
     * @brief Callback function type invoked for each successfully parsed row.
     * @param id The parsed and trimmed ID (moved).
     * @param tags The vector of parsed and trimmed tags (moved).
     */
    using Callback = std::function<void(core::StringId&& id, core::StringTagSet&& tags)>;

    /**
     * @brief Constructs a CsvParser.
     * @param delimiter The character separating columns in the CSV file. Defaults to '|'.
     */
    explicit CsvParser(char delimiter = '|');

    /**
     * @brief Parses an entire file, invoking the callback for each valid row.
     * Skips empty lines or lines containing only whitespace.
     * Logs errors for lines that have content but cannot be parsed correctly (e.g., missing ID).
     *
     * @param filepath Path to the CSV file.
     * @param callback Function to call for each parsed row.
     * @return True if the file was opened successfully, false otherwise. Parsing continues even if some lines have errors.
     */
    bool parseFile(const std::filesystem::path& filepath, const Callback& callback);

    /**
     * @brief Parses data from an input stream, invoking the callback for each valid row.
     * Skips empty lines or lines containing only whitespace.
     * Logs errors for lines that have content but cannot be parsed correctly.
     * Optionally starts parsing from a specific offset (useful for incremental loading).
     * Note: Seeking might not work reliably on all stream types (e.g., std::cin).
     * Note: Seeking to an offset might start reading mid-line; getline will read until the *next* newline.
     *
     * @param stream The input stream to read from.
     * @param callback Function to call for each parsed row.
     * @param start_offset Byte offset to seek to before starting to read (default is 0).
     * @return True if parsing proceeded without fatal stream errors after seeking (if applicable), false otherwise.
     */
    bool parseStream(std::istream& stream, const Callback& callback, core::FileOffset start_offset = 0);

    // Accessors for statistics (optional)
    // uint64_t getProcessedLinesCount() const { return processed_lines_; }
    // uint64_t getErrorLinesCount() const { return error_lines_; }

private:
    /**
     * @brief Parses a single line of text into an ID and a set of tags.
     * Trims whitespace from the ID and tags.
     *
     * @param line The line content as a string_view.
     * @param out_id Output parameter for the parsed ID.
     * @param out_tags Output parameter for the parsed tags.
     * @return True if the line was parsed successfully (non-empty ID found), false otherwise.
     */
    bool parseLine(std::string_view line, core::StringId& out_id, core::StringTagSet& out_tags);

    char delimiter_;
    // uint64_t processed_lines_ = 0;
    // uint64_t error_lines_ = 0;
};

} // namespace io
} // namespace bitmap_index

#endif // BITMAP_INDEX_IO_CSV_PARSER_H_