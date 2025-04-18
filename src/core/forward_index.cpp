#include "bitmap_index/core/forward_index.h"
#include <utility> // For std::move
#include <vector>
#include <fstream> // Include fstream for ofstream/ifstream operations
#include <iostream> // For error logging


namespace bitmap_index::core
{
    // --- Basic Binary I/O Helpers (Simplified) ---
    template<typename T>
    inline bool write_binary(std::ostream& os, const T& value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(T));
        return os.good();
    }

    template<typename T>
    inline bool read_binary(std::istream& is, T& value) {
        is.read(reinterpret_cast<char*>(&value), sizeof(T));
        // Check both stream state and read count
        return is.good() && (is.gcount() == sizeof(T));
    }

    void ForwardIndex::addTags(DocId doc_id, std::vector<TagId>&& tag_ids)
    {
        // std::unique_lock lock(rw_mutex_); // Write lock if concurrent

        if (doc_id == INVALID_DOC_ID) return; // Or throw/log

        if (doc_id >= doc_to_tags_.size())
        {
            // Resize the vector to accommodate the new doc_id.
            // Initialize intermediate elements with empty vectors.
            doc_to_tags_.resize(static_cast<size_t>(doc_id) + 1);
        }
        // Move the provided vector into the storage
        doc_to_tags_[doc_id] = std::move(tag_ids);
    }

    void ForwardIndex::addTags(DocId doc_id, const std::vector<TagId>& tag_ids)
    {
        // std::unique_lock lock(rw_mutex_); // Write lock if concurrent

        if (doc_id == INVALID_DOC_ID) return;

        if (doc_id >= doc_to_tags_.size())
        {
            doc_to_tags_.resize(static_cast<size_t>(doc_id) + 1);
        }
        // Copy the provided vector
        doc_to_tags_[doc_id] = tag_ids;
    }

    void ForwardIndex::addTag(DocId doc_id, TagId tag_id)
    {
        // std::unique_lock lock(rw_mutex_); // Write lock if concurrent

        if (doc_id == INVALID_DOC_ID || tag_id == INVALID_TAG_ID) return;

        if (doc_id >= doc_to_tags_.size())
        {
            doc_to_tags_.resize(static_cast<size_t>(doc_id) + 1);
        }
        // Add the tag to the vector for this doc_id
        // Note: This allows duplicate TagIds for a DocId. If uniqueness is needed,
        // check for existence before pushing back or use std::set/unordered_set internally.
        doc_to_tags_[doc_id].push_back(tag_id);
    }


    const std::vector<TagId>& ForwardIndex::getTags(DocId doc_id) const
    {
        // std::shared_lock lock(rw_mutex_); // Read lock if concurrent

        if (doc_id == INVALID_DOC_ID || doc_id >= doc_to_tags_.size())
        {
            return empty_vector_; // Return ref to the static empty vector
        }
        return doc_to_tags_[doc_id];
    }

    size_t ForwardIndex::getDocCount() const
    {
        // std::shared_lock lock(rw_mutex_); // Read lock if concurrent
        return doc_to_tags_.size();
    }

    void ForwardIndex::clear()
    {
        // Clear the main data structure. The const empty_vector_ remains unchanged.
        doc_to_tags_.clear();
        // If using shrink_to_fit is desired:
        // doc_to_tags_.shrink_to_fit();
    }

    bool ForwardIndex::save(std::ostream& os) const {
        // std::shared_lock lock(rw_mutex_); // Read lock

        try {
            uint64_t num_docs = doc_to_tags_.size();
            if (!write_binary(os, num_docs)) return false;

            for (const auto& tag_id_vector : doc_to_tags_) {
                uint64_t num_tags = tag_id_vector.size();
                if (!write_binary(os, num_tags)) return false;
                for (const TagId& tag_id : tag_id_vector) {
                    if (!write_binary(os, tag_id)) return false;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error saving ForwardIndex: " << e.what() << std::endl;
            return false;
        }
        return os.good();
    }

    bool ForwardIndex::load(std::istream& is) {
        // std::unique_lock lock(rw_mutex_); // Write lock
        clear(); // Start fresh

        try {
            uint64_t num_docs = 0;
            if (!read_binary(is, num_docs)) {
                 // Check if it was just an empty file (EOF before reading)
                 if (is.eof() && is.gcount() == 0) return true;
                 std::cerr << "Error reading number of documents for ForwardIndex." << std::endl;
                 return false;
            }

            doc_to_tags_.resize(static_cast<size_t>(num_docs));

            for (uint64_t i = 0; i < num_docs; ++i) {
                uint64_t num_tags = 0;
                if (!read_binary(is, num_tags)) {
                    std::cerr << "Error reading number of tags for doc " << i << std::endl;
                    return false;
                }
                doc_to_tags_[i].reserve(static_cast<size_t>(num_tags));
                for (uint64_t j = 0; j < num_tags; ++j) {
                    TagId tag_id = INVALID_TAG_ID; // Initialize
                    if (!read_binary(is, tag_id)) {
                         std::cerr << "Error reading tag " << j << " for doc " << i << std::endl;
                         return false;
                    }
                    doc_to_tags_[i].push_back(tag_id);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading ForwardIndex: " << e.what() << std::endl;
            clear(); // Clear potentially partial load
            return false;
        }
        return is.good() || is.eof(); // Allow EOF after reading expected data
    }
} // namespace bitmap_index::core
