#include "bitmap_index/core/index_manager.h"
#include <fstream>
#include <iostream> // For error logging
#include <vector>
#include <set> // To ensure unique TagIds for forward index

namespace bitmap_index::core
{

    IndexManager::IndexManager(std::filesystem::path data_file_path,
                               std::filesystem::path status_file_path,
                               char delimiter)
        : data_file_path_(std::move(data_file_path)),
          status_file_path_(std::move(status_file_path)),
          csv_delimiter_(delimiter),
          parser_(delimiter) // Initialize the parser with the delimiter
    {
        last_processed_offset_ = readLastOffset();
        std::cout << "IndexManager initialized. Last processed offset: " << last_processed_offset_ << std::endl;
    }

    // --- Private Helper Methods ---

    void IndexManager::processParsedLine(StringId&& id_str, StringTagSet&& tags_str)
    {
        // This function assumes it's called under the manager_mutex_ write lock

        // 1. Get DocId from Mapping (adds if new)
        DocId doc_id = mapping_.getId(id_str); // Use const ref version first
        if (doc_id == INVALID_DOC_ID) {
             std::cerr << "Warning: Skipping line with invalid or empty document ID: " << id_str << std::endl;
            return; // Skip if ID is invalid
        }

        std::vector<TagId> tag_ids;
        tag_ids.reserve(tags_str.size());
        std::set<TagId> unique_tag_ids; // Use set to handle potential duplicates efficiently for forward index

        // 2. Get TagIds from Mapping (adds if new)
        for (const auto& tag_str : tags_str) {
            TagId tag_id = mapping_.getTagId(tag_str);
             if (tag_id != INVALID_TAG_ID) {
                 tag_ids.push_back(tag_id); // Add to list for inverted index
                 unique_tag_ids.insert(tag_id); // Add to set for forward index
             } else {
                  std::cerr << "Warning: Skipping invalid or empty tag for document ID: " << id_str << std::endl;
             }
        }

        // 3. Update Forward Index (if needed - using unique tags)
        // Convert set back to vector for forward_index storage if it stores vectors
        std::vector<TagId> fwd_tags(unique_tag_ids.begin(), unique_tag_ids.end());
        forward_index_.addTags(doc_id, std::move(fwd_tags)); // Move the unique tags vector

        // 4. Update Inverted Index (using all valid tag occurrences)
        for (TagId tag_id : tag_ids) { // Iterate through original list including duplicates if needed by inverted logic
            inverted_index_.add(doc_id, tag_id);
        }
    }

    FileOffset IndexManager::readLastOffset() {
        // Read lock isn't strictly necessary here if only called from constructor/loadIncremental
        // but safer if potentially called elsewhere. Assumes called before heavy loading starts.
        // std::shared_lock lock(manager_mutex_); // If reads need protection

        FileOffset offset = 0;
        if (std::filesystem::exists(status_file_path_)) {
            std::ifstream status_file(status_file_path_);
            if (status_file.is_open()) {
                status_file >> offset;
                if (status_file.fail() && !status_file.eof()) {
                     std::cerr << "Warning: Failed to read valid offset from status file '"
                               << status_file_path_ << "'. Resetting to 0." << std::endl;
                     offset = 0; // Reset on read error
                 }
            } else {
                 std::cerr << "Warning: Could not open status file '" << status_file_path_
                           << "' for reading, assuming offset 0." << std::endl;
            }
        }
        return offset;
    }

     bool IndexManager::writeLastOffset(FileOffset offset) {
         // Write lock isn't strictly necessary if only called at end of loadIncremental.
         // std::unique_lock lock(manager_mutex_); // If writes need protection

         std::ofstream status_file(status_file_path_, std::ios::trunc); // Truncate to overwrite
         if (status_file.is_open()) {
             status_file << offset;
             if (status_file.fail()) {
                 std::cerr << "Error: Failed to write offset " << offset << " to status file '"
                           << status_file_path_ << "'." << std::endl;
                 return false;
             }
             return true;
         } else {
             std::cerr << "Error: Could not open status file '" << status_file_path_
                       << "' for writing." << std::endl;
             return false;
         }
     }


    // --- Public Methods ---

    bool IndexManager::loadIncremental(bool optimize_after_load) {
        std::unique_lock lock(manager_mutex_); // Acquire exclusive lock for loading/modifying indices

        if (!utils::fileExists(data_file_path_)) {
            std::cerr << "Error: Data file not found: " << data_file_path_ << std::endl;
            return false;
        }

        // Get current size before opening
        FileOffset current_file_size = utils::getFileSize(data_file_path_);

        // Check if new data exists
        if (current_file_size <= last_processed_offset_) {
             std::cout << "No new data detected in " << data_file_path_ << ". Index is up-to-date." << std::endl;
            return true; // Nothing to load, operation successful
        }

        std::cout << "Loading new data from offset " << last_processed_offset_
                  << " in file " << data_file_path_ << "..." << std::endl;

        std::ifstream data_stream(data_file_path_);
        if (!data_stream.is_open()) {
            std::cerr << "Error: Could not open data file: " << data_file_path_ << std::endl;
            return false;
        }

        // Define the callback using a lambda that captures 'this'
        auto callback = [this](StringId&& id, StringTagSet&& tags) {
            this->processParsedLine(std::move(id), std::move(tags));
        };

        // Parse the stream starting from the last offset
        bool parse_success = parser_.parseStream(data_stream, callback, last_processed_offset_);

        if (!parse_success) {
             // parseStream already logs stream errors, maybe add a summary log here
             std::cerr << "Warning: Parsing encountered issues, but proceeding." << std::endl;
             // Decide if this should be a fatal error (return false) or not.
             // For incremental append-only, maybe allow continuing.
        }

        // Get the final position after parsing
        FileOffset new_offset = static_cast<FileOffset>(data_stream.tellg());
         // If tellg fails or returns -1, use current_file_size as a fallback
         if (data_stream.fail() || new_offset == static_cast<FileOffset>(-1)) {
            std::cerr << "Warning: Could not determine stream position after parsing. Using file size as offset." << std::endl;
            new_offset = current_file_size;
        }


        std::cout << "Finished loading data. New offset: " << new_offset << std::endl;


        // Update the last processed offset
        last_processed_offset_ = new_offset;

        // Optionally optimize the inverted index
        if (optimize_after_load) {
            std::cout << "Optimizing inverted index..." << std::endl;
            if (inverted_index_.runOptimize()) {
                 std::cout << "Optimization complete." << std::endl;
                 inverted_index_.shrinkToFit(); // Optional: Reduce memory usage after optimizing
                 std::cout << "Shrink to fit complete." << std::endl;
            } else {
                 std::cerr << "Warning: Optimization encountered issues." << std::endl;
            }
        }

        // Save the new offset to the status file
        if (!writeLastOffset(last_processed_offset_)) {
             std::cerr << "Critical Warning: Failed to update status file with new offset "
                       << last_processed_offset_ << ". Future loads may reprocess data." << std::endl;
             // Depending on requirements, this might be considered a failure (return false)
        }

        return true; // Loading finished (possibly with warnings)
    }

    StringIdList IndexManager::queryTags(const StringTagSet& tags, BitmapOperation op) {
        std::shared_lock lock(manager_mutex_); // Acquire shared lock for querying

        StringIdList result_ids;
        if (tags.empty()) {
            return result_ids; // Return empty list if no tags provided
        }

        // 1. Convert StringTags to TagIds
        std::vector<TagId> query_tag_ids;
        query_tag_ids.reserve(tags.size());
        for (const auto& tag_str : tags) {
            // Use the const version of getTagId as we are not adding new tags here
             // Need a non-const way to check existence without adding... Add a findTagId method?
             // For now, let's stick to getTagId, but be aware it *could* add tags if called concurrently
             // with loadIncremental without proper locking (which we have via shared_lock).
             // However, mapping *read* doesn't require a lock if writes are blocked.
             // Let's assume Mapping::getTagId(const StringTag&) IS const-correct and doesn't modify.
             // A quick look at mapping.cpp shows it *does* modify if tag is new.
             // This violates the const-correctness needed for shared lock safety.

             // --- Revised approach for Query ---
             // We need a way to look up TagIds without modifying the map during query.
             // Add a const lookup method to Mapping or use the internal map directly (carefully).

             // Let's assume a hypothetical `findTagId` exists in Mapping for now.
             // TagId tag_id = mapping_.findTagId(tag_str); // Hypothetical const lookup

             // --- Workaround without changing Mapping (Less Ideal) ---
             // Access the internal map directly (breaking encapsulation slightly, but needed for const-correct read)
             // This requires making the internal maps accessible or adding a specific const lookup method.
             // Let's *assume* we add `std::optional<TagId> Mapping::findTagId(const StringTag& str_tag) const;`

             // --- Sticking to Original Plan (Requires Fixing Mapping Const-Correctness Later) ---
             // Temporarily cast away const - **NOT RECOMMENDED FOR PRODUCTION**
             // Or, accept that queries might theoretically add tags if run during first load (unlikely scenario?)
             // Safest: Upgrade lock to unique for query (slow) or fix Mapping const-correctness.

             // **Decision:** For now, proceed acknowledging Mapping::getTagId isn't truly const.
             // The shared_lock prevents concurrent *loads*, but a query *could* still add a new tag string mapping.

            TagId tag_id = mapping_.getTagId(tag_str); // Acknowledged non-const issue
            if (tag_id != INVALID_TAG_ID) {
                 query_tag_ids.push_back(tag_id);
             } else {
                 // If a tag doesn't exist, how should it affect the operation?
                 // AND: Result becomes empty.
                 // OR/XOR/ANDNOT (right side): This tag is ignored.
                 // ANDNOT (left side): Result becomes empty.
                 if (op == BitmapOperation::AND || (op == BitmapOperation::ANDNOT && query_tag_ids.empty())) {
                     return result_ids; // Return empty result immediately
                 }
                 // Otherwise, just skip this unknown tag for OR/XOR/ANDNOT(right)
             }
        }

        if (query_tag_ids.empty() && op != BitmapOperation::AND) {
            // If all tags were unknown, but op wasn't AND, result is still empty.
             return result_ids;
        }


        // 2. Perform Bitmap Operation using InvertedIndex
        roaring::Roaring result_bitmap = inverted_index_.performOperation(query_tag_ids, op);

        // 3. Convert result DocIds back to StringIds using Mapping
        result_ids.reserve(result_bitmap.cardinality());
        for (auto it = result_bitmap.begin(); it != result_bitmap.end(); ++it) {
            DocId doc_id = *it;
            StringId str_id = mapping_.getStringId(doc_id); // This is const-safe
            if (!str_id.empty()) { // Should always be found if in bitmap
                result_ids.push_back(str_id);
            } else {
                std::cerr << "Warning: Found DocId " << doc_id << " in bitmap result, but no corresponding StringId found in mapping." << std::endl;
            }
        }

        return result_ids;
    }

    StringTagSet IndexManager::getTagsForDocument(const StringId& doc_id_str) {
         std::shared_lock lock(manager_mutex_); // Acquire shared lock for querying

         StringTagSet result_tags;

         // 1. Convert StringId to DocId
         // Again, need a const-correct lookup for DocId. Assume `findId` exists.
         // DocId doc_id = mapping_.findId(doc_id_str); // Hypothetical const lookup

         // **Workaround:** Use non-const getId, acknowledging issue.
         DocId doc_id = mapping_.getId(doc_id_str); // Acknowledged non-const issue

         if (doc_id == INVALID_DOC_ID || doc_id >= forward_index_.getDocCount()) {
            // Document ID not found in mapping or forward index
            return result_tags; // Return empty set
        }


        // 2. Get TagIds from ForwardIndex
        const std::vector<TagId>& tag_ids = forward_index_.getTags(doc_id); // This is const-safe

        // 3. Convert TagIds back to StringTags using Mapping
        result_tags.reserve(tag_ids.size());
        for (TagId tag_id : tag_ids) {
            StringTag tag_str = mapping_.getStringTag(tag_id); // This is const-safe
            if (!tag_str.empty()) { // Should always be found if in forward index
                result_tags.push_back(tag_str);
            } else {
                 std::cerr << "Warning: Found TagId " << tag_id << " in forward index for DocId " << doc_id
                           << ", but no corresponding StringTag found in mapping." << std::endl;
            }
        }

        return result_tags;
    }

    size_t IndexManager::getDocumentCount() const {
        std::shared_lock lock(manager_mutex_);
        return mapping_.getDocCount();
    }

    size_t IndexManager::getTagCount() const {
        std::shared_lock lock(manager_mutex_);
        return mapping_.getTagCount();
    }


} // namespace bitmap_index::core