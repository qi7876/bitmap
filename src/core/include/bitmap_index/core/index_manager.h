#ifndef BITMAP_INDEX_CORE_INDEX_MANAGER_H_
#define BITMAP_INDEX_CORE_INDEX_MANAGER_H_

#include "bitmap_index/core/types.h"
#include "bitmap_index/core/mapping.h"
#include "bitmap_index/core/forward_index.h"
#include "bitmap_index/core/inverted_index.h"
#include "bitmap_index/io/csv_parser.h" // Include CsvParser header
#include "bitmap_index/utils/file_util.h" // For file size checking
#include <filesystem>
#include <string>
#include <vector>
#include <shared_mutex> // For thread safety

namespace bitmap_index
{
    namespace core
    {

        /**
         * @brief Manages the overall bitmap index, coordinating loading, mapping, and querying.
         *
         * Owns the Mapping, ForwardIndex, and InvertedIndex instances.
         * Provides methods to load data from CSV files incrementally and perform queries.
         * Handles the translation between string representations and internal IDs.
         */
        class IndexManager
        {
        public:
            /**
             * @brief Constructs an IndexManager.
             * @param data_file_path The path to the main CSV data file.
             * @param status_file_path Path to a file used to store the last processed offset.
             * @param delimiter The delimiter used in the CSV file (default: '|').
             */
            explicit IndexManager(std::filesystem::path data_file_path,
                                  std::filesystem::path status_file_path = "index_status.txt",
                                  char delimiter = '|');

            /**
             * @brief Loads new data from the data file since the last load.
             * Reads the last processed offset from the status file, processes new lines,
             * and updates the status file with the new offset.
             *
             * @param optimize_after_load If true, run optimization on the inverted index after loading.
             * @return True if loading was successful (even if no new data), false on critical errors.
             */
            bool loadIncremental(bool optimize_after_load = true);

            /**
             * @brief Performs a query based on a set of tags and an operation.
             *
             * @param tags The set of string tags for the query.
             * @param op The bitmap operation to perform (AND, OR, XOR, ANDNOT).
             * @return A list of string IDs (DocIds) matching the query criteria.
             */
            StringIdList queryTags(const StringTagSet& tags, BitmapOperation op);

            /**
             * @brief Retrieves the set of string tags associated with a given document ID.
             *
             * @param doc_id_str The string ID of the document.
             * @return A vector of string tags associated with the document. Returns an empty vector
             * if the document ID is not found.
             */
            StringTagSet getTagsForDocument(const StringId& doc_id_str);

            /**
             * @brief Gets the total number of unique documents indexed.
             */
            size_t getDocumentCount() const;

            /**
             * @brief Gets the total number of unique tags indexed.
             */
            size_t getTagCount() const;

            // --- TODO: Persistence Methods (Optional) ---
            // bool saveIndex(const std::filesystem::path& directory);
            // bool loadIndex(const std::filesystem::path& directory);

        private:
            /**
             * @brief Callback function passed to the CsvParser.
             * Handles mapping strings to IDs and updating forward/inverted indices.
             */
            void processParsedLine(StringId&& id_str, StringTagSet&& tags_str);

            /**
             * @brief Reads the last processed offset from the status file.
             */
            FileOffset readLastOffset();

            /**
             * @brief Writes the current file offset to the status file.
             */
            bool writeLastOffset(FileOffset offset);

            std::filesystem::path data_file_path_;
            std::filesystem::path status_file_path_;
            char csv_delimiter_;
            FileOffset last_processed_offset_ = 0;

            Mapping mapping_;
            ForwardIndex forward_index_;
            InvertedIndex inverted_index_;
            io::CsvParser parser_;

            // Mutex for protecting access during loading and querying
            // Use std::unique_lock for writes (loadIncremental), std::shared_lock for reads (queries)
            mutable std::shared_mutex manager_mutex_;
        };

    } // namespace core
} // namespace bitmap_index

#endif // BITMAP_INDEX_CORE_INDEX_MANAGER_H_