#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_tostring.hpp" // Include for StringMaker
#include "bitmap_index/io/csv_parser.h"
#include "bitmap_index/core/types.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iterator> // For std::ostream_iterator
#include <algorithm> // For std::copy

namespace fs = std::filesystem;
using namespace bitmap_index::core;
using namespace bitmap_index::io;

// --- Catch2 StringMaker Specializations ---

// Helper to print vector elements
namespace Catch {
    template<>
    struct StringMaker<std::vector<std::string>> {
        static std::string convert(const std::vector<std::string>& v) {
            std::stringstream ss;
            ss << "{ ";
            if (!v.empty()) {
                std::copy(v.begin(), v.end() - 1, std::ostream_iterator<std::string>(ss, "\", \""));
                ss << "\"" << v.back() << "\"";
            }
            ss << " }";
            return ss.str();
        }
    };

    // Helper to print map elements
    template<>
    struct StringMaker<std::map<std::string, std::vector<std::string>>> {
        static std::string convert(const std::map<std::string, std::vector<std::string>>& m) {
            std::stringstream ss;
            ss << "{ ";
            bool first = true;
            for (const auto& pair : m) {
                if (!first) {
                    ss << ", ";
                }
                ss << "{\"" << pair.first << "\": " << StringMaker<std::vector<std::string>>::convert(pair.second) << "}";
                first = false;
            }
            ss << " }";
            return ss.str();
        }
    };
} // namespace Catch

// --- End Catch2 StringMaker Specializations ---

// --- Test Helper ---
// Simple RAII helper for temporary files (can be shared or redefined)
class TestFile {
public:
    TestFile(const fs::path& path, const std::string& content) : path_(path) {
        std::ofstream outfile(path_);
        outfile << content;
        outfile.flush(); // Explicitly flush buffers
        outfile.close(); // Explicitly close the file
        // Destructor will still run but file is already closed
    }
    ~TestFile() {
        std::error_code ec;
        fs::remove(path_, ec); // Ignore error on cleanup
    }
    const fs::path& getPath() const { return path_; }
private:
    fs::path path_;
};
// --- End Test Helper ---


TEST_CASE("CsvParser Tests", "[io][csv_parser]") {

    const fs::path test_dir = "csv_parser_test_files"; // Subdir for test files
    fs::create_directory(test_dir); // Ensure it exists

    // --- Callback Setup ---
    // Use a map to store results captured by the callback
    std::map<StringId, StringTagSet> parsed_data;
    // Define the callback lambda
    // Define the callback lambda
    auto callback = [&](StringId&& id, StringTagSet&& tags) {
        REQUIRE(!id.empty());

        // --- SAFER INSERTION ---
        // 1. Create a copy of the ID to use as the key.
        //    This ensures the key used for map lookup/insertion is stable
        //    and not affected by the subsequent move of the original 'id'.
        StringId key_copy = id;

        // 2. Use the key copy for map access and move only the value (tags).
        parsed_data[key_copy] = std::move(tags);

        // --- OLD PROBLEMATIC LINE ---
        // parsed_data[std::move(id)] = std::move(tags);
    };

    // --- Test Sections ---

    SECTION("Basic Parsing with default delimiter '|'") {
        const fs::path file_path = test_dir / "basic.csv";
        const std::string content =
            "id1 | tag1 | tag2\n"
            "id2 | tag3\n"
            "id3 | tag1 | tag4 | tag5\n";
        TestFile test_file(file_path, content);

        CsvParser parser; // Default delimiter '|'
        parsed_data.clear(); // Reset results

        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {"tag1", "tag2"}},
            {"id2", {"tag3"}},
            {"id3", {"tag1", "tag4", "tag5"}}
        };
        REQUIRE(parsed_data == expected);
    }

    SECTION("Whitespace Handling") {
        const fs::path file_path = test_dir / "whitespace.csv";
        const std::string content =
            "  id1 | tag1 |  tag2  \n" // Leading/trailing space in fields/line
            "id2 |tag3 \n"             // No space after delimiter
            "\n"                       // Empty line
            "   \t   \n"               // Whitespace only line
            "id3| tag4 |tag5\n";       // Mixed spacing
        TestFile test_file(file_path, content);

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {"tag1", "tag2"}},
            {"id2", {"tag3"}},
            {"id3", {"tag4", "tag5"}}
        };
        REQUIRE(parsed_data == expected);
    }

    SECTION("Custom Delimiter ','") {
        const fs::path file_path = test_dir / "custom_delim.csv";
        const std::string content =
            "id1,tag1,tag2\n"
            "id2,tag3\n";
        TestFile test_file(file_path, content);

        CsvParser parser(','); // Custom delimiter
        parsed_data.clear();

        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {"tag1", "tag2"}},
            {"id2", {"tag3"}}
        };
        REQUIRE(parsed_data == expected);
    }

    SECTION("Handling Empty Tags") {
        const fs::path file_path = test_dir / "empty_tags.csv";
        // Assumes parser skips empty tags after trimming
        const std::string content =
            "id1 | tag1 || tag3\n" // Empty tag between tag1 and tag3
            "id2 | | tag4\n"      // Leading empty tag
            "id3 | tag5 | \n";    // Trailing empty tag
        TestFile test_file(file_path, content);

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {"tag1", "tag3"}},
            {"id2", {"tag4"}},
            {"id3", {"tag5"}}
        };
        REQUIRE(parsed_data == expected);
    }

     SECTION("ID Only (No Tags)") {
        const fs::path file_path = test_dir / "id_only.csv";
        const std::string content =
            "id1\n"          // ID only, no delimiter
            "id2 |\n"        // ID with delimiter but no tags
            "id3 | \n";      // ID with delimiter and whitespace tag (skipped)
        TestFile test_file(file_path, content);

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {}},
            {"id2", {}},
            {"id3", {}}
        };
        REQUIRE(parsed_data == expected);
    }

    SECTION("Malformed Lines (Skipped)") {
        const fs::path file_path = test_dir / "malformed.csv";
        const std::string content =
            "id1 | tag1\n"
            " | tag2 | tag3\n" // Missing ID
            "id2 | tag4\n"
            "   | tag5\n";     // Whitespace ID (treated as missing)
        TestFile test_file(file_path, content);

        CsvParser parser('|');
        parsed_data.clear();

        // parseFile should still return true, but skip bad lines (and print warnings)
        REQUIRE(parser.parseFile(file_path, callback));

        std::map<StringId, StringTagSet> expected = {
            {"id1", {"tag1"}},
            {"id2", {"tag4"}}
        };
        // Check that only the valid lines were processed
        REQUIRE(parsed_data == expected);
        // We could try to capture stderr here, but it's often complex in tests.
        // Checking the resulting data is the primary goal.
    }

    SECTION("Empty File") {
        const fs::path file_path = test_dir / "empty.csv";
        TestFile test_file(file_path, ""); // Empty content

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseFile(file_path, callback));
        REQUIRE(parsed_data.empty()); // No data should be parsed
    }

    SECTION("Non-Existent File") {
        const fs::path file_path = test_dir / "non_existent.csv";
        // Ensure file does not exist
        fs::remove(file_path);

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE_FALSE(parser.parseFile(file_path, callback)); // Should fail to open
        REQUIRE(parsed_data.empty()); // No data should have been parsed
    }

    SECTION("Stream Parsing") {
        const std::string content =
            "sid1 | stag1 | stag2\n"
            "sid2 | stag3\n";
        std::stringstream ss(content);

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseStream(ss, callback)); // Parse from stream

        std::map<StringId, StringTagSet> expected = {
            {"sid1", {"stag1", "stag2"}},
            {"sid2", {"stag3"}}
        };
        REQUIRE(parsed_data == expected);
    }

     SECTION("Stream Parsing with Offset") {
        const std::string content =
            "line_to_skip | tagA\n" // This line should be skipped
            "line1 | tag1 | tag2\n"
            "line2 | tag3\n";
        std::stringstream ss(content);

        // Calculate offset after the first line
        size_t offset = content.find('\n');
        REQUIRE(offset != std::string::npos); // Ensure newline was found
        offset += 1; // Start reading *after* the newline

        CsvParser parser('|');
        parsed_data.clear();

        REQUIRE(parser.parseStream(ss, callback, static_cast<FileOffset>(offset)));

        std::map<StringId, StringTagSet> expected = {
            {"line1", {"tag1", "tag2"}},
            {"line2", {"tag3"}}
        };
        REQUIRE(parsed_data == expected);
    }


    // --- Cleanup ---
    // Remove the test directory
    fs::remove_all(test_dir);
}