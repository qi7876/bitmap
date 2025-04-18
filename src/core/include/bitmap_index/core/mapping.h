#ifndef BITMAP_INDEX_CORE_MAPPING_H_
#define BITMAP_INDEX_CORE_MAPPING_H_

#include "bitmap_index/core/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex> // Include mutex for potential future thread-safety

namespace bitmap_index::core
{
    /**
     * @brief Manages the bidirectional mapping between string IDs/tags and internal numeric IDs.
     *
     * This class handles the conversion between human-readable strings used in input
     * (StringId, StringTag) and the numeric IDs (DocId, TagId) used internally,
     * particularly by Roaring Bitmaps. It ensures that each unique string ID/tag
     * corresponds to a unique numeric ID.
     *
     * Note: This initial version is not thread-safe for concurrent writes.
     * If multiple threads build the index concurrently, external locking or
     * internal locking (e.g., using the provided mutex) is required. Reads after
     * the index is built are generally safe without locking if the maps are not
     * modified further.
     */
    class Mapping
    {
    public:
        Mapping() = default;

        // --- DocId Mapping ---

        /**
         * @brief Gets the numeric DocId for a given string ID.
         * If the string ID is new, assigns the next available DocId and stores the mapping.
         * @param str_id The string ID to look up or add.
         * @return The corresponding DocId. Returns INVALID_DOC_ID if the input is empty.
         */
        DocId getId(const StringId& str_id);

        /**
        * @brief Gets the numeric DocId for a given string ID (move semantics).
        * If the string ID is new, assigns the next available DocId and stores the mapping.
        * @param str_id The string ID to look up or add (will be moved from).
        * @return The corresponding DocId. Returns INVALID_DOC_ID if the input is empty.
        */
        DocId getId(StringId&& str_id);


        /**
         * @brief Retrieves the string ID corresponding to a given numeric DocId.
         * @param doc_id The numeric DocId to look up.
         * @return The corresponding StringId, or an empty string if the DocId is invalid or not found.
         */
        StringId getStringId(DocId doc_id) const; // Read-only, potentially thread-safer

        /**
         * @brief Gets the next available DocId that would be assigned.
         * Useful for knowing the current size of the DocId space.
         * @return The next DocId to be assigned.
         */
        DocId getNextDocId() const; // Read-only


        // --- TagId Mapping ---

        /**
         * @brief Gets the numeric TagId for a given string tag.
         * If the string tag is new, assigns the next available TagId and stores the mapping.
         * @param str_tag The string tag to look up or add.
         * @return The corresponding TagId. Returns INVALID_TAG_ID if the input is empty.
         */
        TagId getTagId(const StringTag& str_tag);

        /**
         * @brief Gets the numeric TagId for a given string tag (move semantics).
         * If the string tag is new, assigns the next available TagId and stores the mapping.
         * @param str_tag The string tag to look up or add (will be moved from).
         * @return The corresponding TagId. Returns INVALID_TAG_ID if the input is empty.
         */
        TagId getTagId(StringTag&& str_tag);


        /**
         * @brief Retrieves the string tag corresponding to a given numeric TagId.
         * @param tag_id The numeric TagId to look up.
         * @return The corresponding StringTag, or an empty string if the TagId is invalid or not found.
         */
        StringTag getStringTag(TagId tag_id) const; // Read-only

        /**
         * @brief Gets the next available TagId that would be assigned.
         * Useful for knowing the current size of the TagId space.
         * @return The next TagId to be assigned.
         */
        TagId getNextTagId() const; // Read-only


        // --- Utility ---

        /**
         * @brief Returns the number of unique documents (StringIds) currently mapped.
         */
        size_t getDocCount() const;

        /**
         * @brief Returns the number of unique tags (StringTags) currently mapped.
         */
        size_t getTagCount() const;

        /**
         * @brief Clears all mapping data.
         */
        void clear(); // <--- ADD THIS DECLARATION

        /**
         * @brief Saves mapping data (vectors only) to a binary output stream.
         * Format: [doc_vector_size] ([string_len] [string_bytes]...)
         * [tag_vector_size] ([string_len] [string_bytes]...)
         * @param os Output stream.
         * @return True on success.
         */
        bool save(std::ostream& os) const;

        /**
         * @brief Loads mapping data (vectors) from a binary input stream.
         * Rebuilds the reverse maps after loading vectors. Clears existing data.
         * @param is Input stream.
         * @return True on success.
         */
        bool load(std::istream& is);

    private:
        // Using vectors for DocId/TagId -> String mapping (fast index access)
        std::vector<StringId> doc_id_to_string_;
        std::vector<StringTag> tag_id_to_string_;

        // Using unordered_map for String -> DocId/TagId mapping (fast hash lookup)
        std::unordered_map<StringId, DocId> string_to_doc_id_;
        std::unordered_map<StringTag, TagId> string_to_tag_id_;

        // Mutex for thread-safety if needed in the future ( uncomment if used )
        // mutable std::mutex map_mutex_;
    };
} // namespace bitmap_index::core

#endif // BITMAP_INDEX_CORE_MAPPING_H_
