#ifndef BITMAP_INDEX_CORE_FORWARD_INDEX_H_
#define BITMAP_INDEX_CORE_FORWARD_INDEX_H_

#include "bitmap_index/core/types.h"
#include <vector>
#include <iosfwd> // For std::ostream, std::istream
#include <shared_mutex> // For potential future read/write locking

namespace bitmap_index
{
    namespace core
    {
        /**
         * @brief Stores the mapping from DocId to the set of TagIds associated with it.
         *
         * Allows efficient retrieval of all tags for a given document.
         * This implementation uses a vector of vectors. The outer vector is indexed by DocId.
         *
         * Note: This initial version assumes writes happen before reads or are externally synchronized.
         * For concurrent read/write access, internal locking (like the commented-out shared_mutex)
         * would be needed.
         */
        class ForwardIndex
        {
        public:
            ForwardIndex() = default;

            /**
             * @brief Associates a list of tags with a document ID.
             *
             * If the doc_id is beyond the current size, the underlying storage will be resized.
             * If the doc_id already has tags, this will overwrite the existing tags.
             * Consider using an `addTag(DocId, TagId)` method if incremental addition is needed.
             *
             * @param doc_id The document ID.
             * @param tag_ids A vector of TagIds associated with the document.
             */
            void addTags(DocId doc_id, std::vector<TagId>&& tag_ids);
            void addTags(DocId doc_id, const std::vector<TagId>& tag_ids); // Overload for const refs

            /**
             * @brief Adds a single tag to a document ID.
             *
             * If the doc_id is beyond the current size, the underlying storage will be resized.
             * If the tag already exists for the document, it is not added again (behavior depends
             * on whether duplicates are desired/handled later). For simplicity, we add it;
             * consider using a set internally if uniqueness is strictly required here.
             *
             * @param doc_id The document ID.
             * @param tag_id The TagId to associate with the document.
             */
            void addTag(DocId doc_id, TagId tag_id);


            /**
             * @brief Retrieves the list of TagIds associated with a given DocId.
             *
             * @param doc_id The document ID to query.
             * @return A constant reference to the vector of TagIds. Returns an empty vector
             * if the doc_id is invalid or has no associated tags.
             */
            const std::vector<TagId>& getTags(DocId doc_id) const;

            /**
             * @brief Returns the number of documents currently stored in the index.
             * This corresponds to the highest DocId + 1 for which tags have been added.
             */
            size_t getDocCount() const;

            /**
             * @brief Clears all data from the forward index.
             */
            void clear(); // <--- ADD THIS

            /**
             * @brief Saves the forward index to a binary output stream.
             * Format: [uint64_t num_docs] ([uint64_t num_tags_for_doc_i] [TagId...]...)
             * @param os Output stream.
             * @return True on success.
             */
            bool save(std::ostream& os) const;
            /**
             * @brief Loads the forward index from a binary input stream. Clears existing data.
             * @param is Input stream.
             * @return True on success.
             */
            bool load(std::istream& is);

        private:
            std::vector<std::vector<TagId>> doc_to_tags_;
            // This const member causes the assignment operator issue
            const std::vector<TagId> empty_vector_ = {}; // Static empty vector for safe returns
            // mutable std::shared_mutex rw_mutex_;
        };
    } // namespace core
} // namespace bitmap_index

#endif // BITMAP_INDEX_CORE_FORWARD_INDEX_H_
