#include "catch2/catch_test_macros.hpp" // Main Catch2 header for tests
#include "bitmap_index/utils/string_util.h" // Header for the code we are testing
#include <vector>
#include <string>

using namespace bitmap_index::utils;

TEST_CASE("String splitting", "[string_util][split]") {

    SECTION("Basic splitting") {
        std::string s = "a|b|c";
        std::vector<std::string> expected = {"a", "b", "c"};
        REQUIRE(split(s, '|') == expected);
    }

    SECTION("Splitting with empty tokens") {
        std::string s = "a||c";
        std::vector<std::string> expected = {"a", "", "c"};
        REQUIRE(split(s, '|') == expected);
    }

    SECTION("Splitting with leading delimiter") {
        std::string s = "|b|c";
        std::vector<std::string> expected = {"", "b", "c"};
        REQUIRE(split(s, '|') == expected);
    }

    SECTION("Splitting with trailing delimiter") {
        std::string s = "a|b|";
        std::vector<std::string> expected = {"a", "b", ""};
        REQUIRE(split(s, '|') == expected);
    }

    SECTION("Splitting with only delimiter") {
        std::string s = "|";
        std::vector<std::string> expected = {"", ""};
        REQUIRE(split(s, '|') == expected);
    }

     SECTION("Splitting with multiple delimiters") {
        std::string s = "a,,b,"; // Using comma as delimiter
        std::vector<std::string> expected = {"a", "", "b", ""};
        REQUIRE(split(s, ',') == expected);
    }

    SECTION("Splitting empty string") {
        std::string s = "";
        std::vector<std::string> expected = {""}; // Split on empty string results in one empty token
        REQUIRE(split(s, '|') == expected);
        // If the intention is an empty vector for an empty string input:
        // REQUIRE(split(s, '|').empty()); // Adjust split logic or test expectation
        // Current logic based on find/substr results in {""}
    }

    SECTION("Splitting with skip_empty = true") {
        std::string s = "a||b| |c|";
        std::vector<std::string> expected = {"a", "b", " ", "c"}; // Note: " " is not empty
        REQUIRE(split(s, '|', true) == expected);
    }

    SECTION("Splitting with skip_empty = true and only delimiters") {
        std::string s = "|||";
        std::vector<std::string> expected = {};
        REQUIRE(split(s, '|', true) == expected);
    }

     SECTION("Splitting with skip_empty = true and empty input") {
        std::string s = "";
        std::vector<std::string> expected = {};
        REQUIRE(split(s, '|', true) == expected);
    }
}

TEST_CASE("String trimming (in-place)", "[string_util][trim]") {
    std::string s;

    SECTION("Trim leading whitespace") {
        s = "  hello";
        REQUIRE(trimLeft(s) == "hello");
        REQUIRE(s == "hello"); // Check in-place modification
    }

    SECTION("Trim trailing whitespace") {
        s = "hello  ";
        REQUIRE(trimRight(s) == "hello");
        REQUIRE(s == "hello");
    }

    SECTION("Trim both leading and trailing whitespace") {
        s = "  hello world  ";
        REQUIRE(trim(s) == "hello world");
        REQUIRE(s == "hello world");
    }

    SECTION("Trim string with only whitespace") {
        s = "   \t\n ";
        REQUIRE(trim(s) == "");
        REQUIRE(s == "");
    }

    SECTION("Trim empty string") {
        s = "";
        REQUIRE(trim(s) == "");
        REQUIRE(s == "");
    }

    SECTION("Trim string with no whitespace") {
        s = "no_whitespace";
        REQUIRE(trim(s) == "no_whitespace");
        REQUIRE(s == "no_whitespace");
    }
}

TEST_CASE("String trimming (copy)", "[string_util][trimCopy]") {
    std::string_view sv;
    std::string original;

    SECTION("Trim leading whitespace copy") {
        original = "  hello";
        sv = original;
        REQUIRE(trimLeftCopy(sv) == "hello");
        REQUIRE(original == "  hello"); // Ensure original is unchanged
    }

    SECTION("Trim trailing whitespace copy") {
        original = "hello  ";
        sv = original;
        REQUIRE(trimRightCopy(sv) == "hello");
        REQUIRE(original == "hello  ");
    }

    SECTION("Trim CSV") {
        original = "id3 | tag1 | tag4 | tag5\n";
        sv = original;
        REQUIRE(trimCopy(sv) == "id3 | tag1 | tag4 | tag5");
        REQUIRE(original == "id3 | tag1 | tag4 | tag5\n");
    }

    SECTION("Trim both leading and trailing whitespace copy") {
        original = "  hello world  ";
        sv = original;
        REQUIRE(trimCopy(sv) == "hello world");
        REQUIRE(original == "  hello world  ");
    }

    SECTION("Trim string with only whitespace copy") {
        original = "   \t\n ";
        sv = original;
        REQUIRE(trimCopy(sv) == "");
        REQUIRE(original == "   \t\n ");
    }

    SECTION("Trim empty string copy") {
        original = "";
        sv = original;
        REQUIRE(trimCopy(sv) == "");
        REQUIRE(original == "");
    }

    SECTION("Trim string with no whitespace copy") {
        original = "no_whitespace";
        sv = original;
        REQUIRE(trimCopy(sv) == "no_whitespace");
        REQUIRE(original == "no_whitespace");
    }
}