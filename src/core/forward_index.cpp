#include "bitmap_index/core/forward_index.h"
#include <utility> // For std::move

namespace bitmap_index::core
{
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
} // namespace bitmap_index::core
