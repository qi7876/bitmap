#ifndef BITMAP_INDEX_UTILS_FILE_UTIL_H_
#define BITMAP_INDEX_UTILS_FILE_UTIL_H_

#include <string>
#include <cstdint> // For uint64_t
#include <filesystem> // Requires C++17

namespace bitmap_index {
    namespace utils {

        /**
         * @brief Checks if a file exists at the given path and is a regular file.
         *
         * @param path The path to the file.
         * @return True if the path exists and is a regular file, false otherwise.
         *         Returns false if any filesystem error occurs.
         */
        bool fileExists(const std::filesystem::path& path);

        /**
         * @brief Gets the size of a file in bytes.
         *
         * @param path The path to the file.
         * @return The size of the file in bytes. Returns 0 if the file does not exist,
         *         is not a regular file, or an error occurs.
         */
        uint64_t getFileSize(const std::filesystem::path& path);

        // You might add other utilities later, e.g.:
        // bool directoryExists(const std::filesystem::path& path);
        // bool createDirectory(const std::filesystem::path& path);
        // std::vector<std::string> listDirectory(const std::filesystem::path& path);

    } // namespace utils
} // namespace bitmap_index

#endif // BITMAP_INDEX_UTILS_FILE_UTIL_H_