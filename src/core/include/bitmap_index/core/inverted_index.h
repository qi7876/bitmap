#ifndef BITMAP_INDEX_CORE_INVERTED_INDEX_H_
#define BITMAP_INDEX_CORE_INVERTED_INDEX_H_

#include "bitmap_index/core/types.h"
#include <vector>
#include <roaring/roaring.hh> // Include Roaring header
#include <shared_mutex>       // For potential future read/write locking
#include <optional>

namespace bitmap_index
{
    namespace core
    {
        // Enum to specify the type of bitmap operation
        enum class BitmapOperation
        {
            AND, // Intersection
            OR, // Union
            XOR, // Symmetric Difference
            ANDNOT // Difference (A and not B)
        };

        /**
         * @brief Stores the mapping from TagId to a Roaring Bitmap of DocIds.
         *
         * This is the core structure for efficient tag-based lookups and set operations.
         * It uses a vector where the index corresponds to the TagId, and each element
         * is a Roaring Bitmap containing the DocIds associated with that tag.
         *
         * Note: Similar to ForwardIndex, concurrent writes require locking. Reads after
         * building are generally safe if no more writes occur. Bitmaps themselves have
         * some thread-safety guarantees depending on operations (check Roaring docs).
         */
        class InvertedIndex
        {
        public:
            InvertedIndex() = default;

            /**
             * @brief Adds a document ID to the bitmap associated with a tag ID.
             *
             * If the tag_id is beyond the current size, the underlying storage will be resized.
             * If the tag_id is encountered for the first time, a new Roaring Bitmap is created.
             *
             * @param doc_id The document ID to add.
             * @param tag_id The tag ID whose bitmap should be updated.
             */
            void add(DocId doc_id, TagId tag_id);

            /**
             * @brief Retrieves the Roaring Bitmap for a given tag ID.
             *
             * @param tag_id The tag ID to query.
             * @return An optional containing a const reference to the bitmap if the tag_id exists
             * and has associated documents. Returns std::nullopt if the tag_id is invalid
             * or has no associated documents.
             */
            std::optional<std::reference_wrapper<const roaring::Roaring>> getBitmap(TagId tag_id) const;

            /**
             * @brief Performs a set operation (AND, OR, XOR, ANDNOT) on the bitmaps of the given tags.
             *
             * @param tag_ids A vector of TagIds involved in the operation.
             * @param op The BitmapOperation to perform.
             * @return A new Roaring Bitmap containing the result of the operation.
             * Returns an empty bitmap if tag_ids is empty or if any tag_id is invalid.
             * For ANDNOT, the operation is performed as tags_ids[0] ANDNOT (tags_ids[1] OR tags_ids[2] OR ...).
             */
            roaring::Roaring performOperation(const std::vector<TagId>& tag_ids, BitmapOperation op) const;

            /**
             * @brief Gets the number of unique documents (cardinality) for a specific tag.
             * @param tag_id The tag ID to query.
             * @return The number of documents associated with the tag, or 0 if the tag is invalid.
             */
            uint64_t getCardinality(TagId tag_id) const;


            /**
             * @brief Returns the number of unique tags currently stored in the index.
             * This corresponds to the highest TagId + 1 for which documents have been added.
             */
            size_t getTagCount() const;

            /**
             * @brief Optimizes the memory layout of all bitmaps.
             * Should be called after adding documents is complete.
             * See roaring::Roaring::runOptimize().
             * @return True if the optimization was successful for all bitmaps (or if there are no bitmaps).
             */
            bool runOptimize();

            /**
             * @brief Shrinks the memory capacity of all bitmaps to fit their current size.
             * See roaring::Roaring::shrinkToFit().
             */
            void shrinkToFit();

            /**
             * @brief Saves the inverted index to a binary output stream.
             * Uses Roaring's portable serialization format.
             * Format: [uint64_t num_bitmaps] [bitmap_1_data] [bitmap_2_data] ...
             * where bitmap_data is: [uint32_t size] [byte data...]
             * @param os The output stream to write to.
             * @return True on success, false on failure.
             */
            bool save(std::ostream& os) const;

            /**
             * @brief Loads the inverted index from a binary input stream.
             * Clears existing data before loading.
             * Expects the format written by save().
             * @param is The input stream to read from.
             * @return True on success, false on failure.
             */
            bool load(std::istream& is);

            /**
             * @brief Clears all data from the inverted index.
             */
            void clear(); // <--- ADD THIS DECLARATION

        private:
            // Ensures the internal vector is large enough for the given tag_id,
            // resizing and default-constructing bitmaps if necessary.
            // Returns true if successful and tag_id is valid, false otherwise.
            // NOTE: This non-const method should be called *before* locking if used internally.
            bool ensureTagCapacity(TagId tag_id);

            std::vector<roaring::Roaring> tag_to_bitmap_;
            // mutable std::shared_mutex rw_mutex_; // For future concurrent read/write
        };
    } // namespace core
} // namespace bitmap_index

#endif // BITMAP_INDEX_CORE_INVERTED_INDEX_H_
