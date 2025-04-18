#include "catch2/catch_test_macros.hpp"
#include "bitmap_index/utils/file_util.h" // Header for the code we are testing
#include <filesystem> // For filesystem operations
#include <fstream>    // For creating test files
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace bitmap_index::utils;

// Helper class for managing temporary test files/dirs using RAII
class TestFile
{
public:
    // Create an empty file
    TestFile(const fs::path& path) : path_(path)
    {
        std::ofstream outfile(path_);
        outfile.close(); // Create empty file
    }

    // Create a file with content
    TestFile(const fs::path& path, const std::string& content) : path_(path)
    {
        std::ofstream outfile(path_);
        outfile << content;
        outfile.close();
    }

    ~TestFile()
    {
        std::error_code ec;
        fs::remove(path_, ec); // Ignore error on cleanup
    }

    const fs::path& getPath() const { return path_; }

private:
    fs::path path_;
};

class TestDirectory
{
public:
    TestDirectory(const fs::path& path) : path_(path)
    {
        fs::create_directory(path_);
    }

    ~TestDirectory()
    {
        std::error_code ec;
        fs::remove(path_, ec); // Remove directory
    }

    const fs::path& getPath() const { return path_; }

private:
    fs::path path_;
};


TEST_CASE("File Utilities", "[file_util]")
{
    // Define paths relative to the build/test directory or use absolute paths
    // Using fs::temp_directory_path() is often safer
    // fs::path test_dir = fs::temp_directory_path() / "bitmap_index_tests";
    // fs::create_directories(test_dir); // Ensure base test dir exists

    // Simpler approach: use relative paths assuming tests run from build dir
    const fs::path existing_file_path = "test_existing_file.txt";
    const fs::path empty_file_path = "test_empty_file.txt";
    const fs::path non_existent_path = "test_non_existent_file.dat";
    const fs::path directory_path = "test_directory";
    const std::string file_content = "Hello, World!";

    // --- Test Setup ---
    // Create files/dirs needed for tests using RAII helpers
    TestFile existing_file(existing_file_path, file_content);
    TestFile empty_file(empty_file_path);
    TestDirectory test_dir(directory_path);
    // Ensure non_existent_path really doesn't exist before tests
    fs::remove(non_existent_path);


    // --- fileExists Tests ---
    SECTION("fileExists")
    {
        REQUIRE(fileExists(existing_file.getPath()) == true);
        REQUIRE(fileExists(empty_file.getPath()) == true);
        REQUIRE(fileExists(non_existent_path) == false);
        REQUIRE(fileExists(test_dir.getPath()) == false); // Should return false for directories
    }

    // --- getFileSize Tests ---
    SECTION("getFileSize")
    {
        REQUIRE(getFileSize(existing_file.getPath()) == file_content.length());
        REQUIRE(getFileSize(empty_file.getPath()) == 0);
        REQUIRE(getFileSize(non_existent_path) == 0);
        REQUIRE(getFileSize(test_dir.getPath()) == 0); // Should return 0 for directories
    }

    // --- Cleanup ---
    // RAII helpers TestFile and TestDirectory handle cleanup automatically
    // If not using RAII, manually remove files/dirs here or use Catch2 fixtures
}
