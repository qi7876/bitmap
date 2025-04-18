#include "bitmap_index/utils/file_util.h"
#include <system_error> // For std::error_code

namespace bitmap_index::utils
{
    bool fileExists(const std::filesystem::path& path)
    {
        std::error_code ec; // Non-throwing overload requires error_code
        bool exists = std::filesystem::exists(path, ec);
        if (ec || !exists)
        {
            return false; // Error occurred or path doesn't exist
        }

        bool is_regular = std::filesystem::is_regular_file(path, ec);
        if (ec)
        {
            return false; // Error occurred during check
        }
        return is_regular;
    }

    uint64_t getFileSize(const std::filesystem::path& path)
    {
        std::error_code ec;
        uint64_t size = std::filesystem::file_size(path, ec);

        // file_size throws if path doesn't exist or isn't a regular file,
        // OR returns static_cast<uintmax_t>(-1) and sets ec on other errors.
        // The non-throwing overload returns 0 on error and sets ec.
        if (ec)
        {
            // Check if the error is simply "file not found" or "not a regular file"
            // These aren't exceptional errors for this function's contract (return 0)
            // Other errors (like permission denied) might warrant logging, but still return 0 here.
            // Note: Specific error codes might vary slightly across platforms.
            // Common errors: std::errc::no_such_file_or_directory, std::errc::not_a_directory (if path is dir)
            // We simply return 0 on any error according to the function's contract.
            return 0;
        }

        // Check if it's actually a regular file, as file_size might behave differently
        // for directories etc. depending on the OS. The fileExists check is more robust.
        // Re-checking might be slightly redundant but ensures consistency.
        if (!fileExists(path))
        {
            // Use our existing robust check
            return 0;
        }


        return static_cast<uint64_t>(size);
    }
} // namespace bitmap_index::utils
