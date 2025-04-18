#include "bitmap_index/io/csv_parser.h"
#include "bitmap_index/utils/string_util.h" // For split and trimCopy
#include <fstream>      // For std::ifstream
#include <istream>      // For std::istream
#include <string>
#include <string_view>
#include <vector>
#include <iostream>     // For error logging (std::cerr)

namespace bitmap_index::io {

CsvParser::CsvParser(char delimiter) : delimiter_(delimiter) {}

bool CsvParser::parseFile(const std::filesystem::path& filepath, const Callback& callback) {
    std::ifstream file_stream(filepath);
    if (!file_stream.is_open()) {
        std::cerr << "Error: Could not open file: " << filepath << std::endl;
        return false;
    }
    // Reset statistics if needed
    // processed_lines_ = 0;
    // error_lines_ = 0;
    return parseStream(file_stream, callback, 0); // Start from beginning
}

bool CsvParser::parseStream(std::istream& stream, const Callback& callback, core::FileOffset start_offset) {
    if (start_offset > 0) {
        stream.seekg(start_offset);
        if (!stream) {
            std::cerr << "Error: Failed to seek to offset " << start_offset << " in stream." << std::endl;
            return false; // Seek failed, cannot proceed reliably
        }
        // Note: If seek lands mid-line, getline will read from there until the next newline.
        // This might skip the partial line, which is often acceptable for append-only logs.
    }

    std::string line_buffer;
    core::StringId current_id;
    core::StringTagSet current_tags;
    // uint64_t line_number = 0; // Optional: for more detailed error messages

    while (std::getline(stream, line_buffer)) {
        // line_number++;
        // processed_lines_++;

        // Trim the whole line first to easily detect empty/whitespace-only lines
        std::string trimmed_line = utils::trimCopy(line_buffer);

        if (trimmed_line.empty()) {
            continue; // Skip empty or whitespace-only lines
        }

        if (parseLine(trimmed_line, current_id, current_tags)) {
            // Successfully parsed, invoke callback with moved data
            callback(std::move(current_id), std::move(current_tags));
            // Reset for next use (optional, as move leaves them in valid state)
            current_id.clear();
            current_tags.clear();
        } else {
            // error_lines_++;
            // Log error for non-empty lines that failed parsing
            std::cerr << "Warning: Skipping malformed line: " << line_buffer << std::endl;
            // Optionally include line_number in the error message
        }
    }

    // Check for non-EOF stream errors after the loop
    if (stream.bad()) {
        std::cerr << "Error: A non-recoverable stream error occurred during parsing." << std::endl;
        return false;
    }
    // stream.fail() might be set if getline fails, but eof is also fail state, so check bad() mainly.

    return true; // Reached end of stream (or specified part) without fatal errors
}


bool CsvParser::parseLine(std::string_view line, core::StringId& out_id, core::StringTagSet& out_tags) {
    // Use the split function from string_util
    // Don't skip empty parts initially, as tags might be empty intentionally?
    // Let's stick to the requirement: id | tag1 | tag2 ...
    // We will trim individual parts later.
    std::vector<std::string> parts = utils::split(line, delimiter_, false); // Don't skip empty

    if (parts.empty()) {
        return false; // Should not happen if line wasn't empty/whitespace-only, but check anyway
    }

    // First part is the ID
    out_id = utils::trimCopy(parts[0]);
    if (out_id.empty())
    {
        return false; // ID cannot be empty after trimming
    }
    
    // Process remaining parts as tags
    out_tags.clear();
    out_tags.reserve(parts.size() > 0 ? parts.size() - 1 : 0); // Pre-allocate memory

    for (size_t i = 1; i < parts.size(); ++i) {
        std::string trimmed_tag = utils::trimCopy(parts[i]);
        if (!trimmed_tag.empty()) { // Only add non-empty tags after trimming
            out_tags.push_back(std::move(trimmed_tag));
        }
        // If you need to allow empty tags (e.g., "id | tag1 | | tag3"), remove the !trimmed_tag.empty() check.
        // Based on the example, empty tags seem unlikely/ignored.
    }

    return true; // Successfully parsed at least an ID
}

} // namespace bitmap_index::io