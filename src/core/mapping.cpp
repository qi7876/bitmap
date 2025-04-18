#include "bitmap_index/core/mapping.h"
#include <utility> // For std::move

namespace bitmap_index::core
{
    // --- DocId Mapping Implementation ---

    DocId Mapping::getId(const StringId& str_id)
    {
        if (str_id.empty())
        {
            return INVALID_DOC_ID; // Or    handle as an error/log
        }

        // std::lock_guard<std::mutex> lock(map_mutex_); // Lock if thread-safety needed

        auto it = string_to_doc_id_.find(str_id);
        if (it != string_to_doc_id_.end())
        {
            // Found existing ID
            return it->second;
        }
        else
        {
            // StringId is new, assign next available DocId
            DocId new_id = static_cast<DocId>(doc_id_to_string_.size());
            // Check for potential overflow if DocId space is exhausted
            if (new_id == INVALID_DOC_ID)
            {
                // Handle error: Too many unique documents
                // For now, let's return INVALID_DOC_ID, but logging or throwing might be better
                return INVALID_DOC_ID;
            }
            doc_id_to_string_.push_back(str_id); // Store the string
            string_to_doc_id_[str_id] = new_id; // Store the reverse mapping (uses copy of str_id)
            return new_id;
        }
    }

    DocId Mapping::getId(StringId&& str_id)
    {
        if (str_id.empty())
        {
            return INVALID_DOC_ID;
        }
        // std::lock_guard<std::mutex> lock(map_mutex_); // Lock if thread-safety needed

        auto it = string_to_doc_id_.find(str_id); // Find requires lvalue, doesn't move yet
        if (it != string_to_doc_id_.end())
        {
            return it->second;
        }
        else
        {
            DocId new_id = static_cast<DocId>(doc_id_to_string_.size());
            if (new_id == INVALID_DOC_ID)
            {
                return INVALID_DOC_ID;
            }
            // Move the string into the vector first
            doc_id_to_string_.push_back(std::move(str_id));
            // Use the (now potentially empty) str_id key - this works because
            // the hash/equality depends on the *original* value before the move.
            // Then, insert into map - IMPORTANT: use the string *stored in the vector* for map key
            string_to_doc_id_[doc_id_to_string_.back()] = new_id;
            return new_id;
        }
    }


    StringId Mapping::getStringId(DocId doc_id) const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_); // Lock if reading concurrently with writes

        if (doc_id < doc_id_to_string_.size())
        {
            return doc_id_to_string_[doc_id];
        }
        return ""; // Return empty string for invalid ID
    }

    DocId Mapping::getNextDocId() const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_); // Lock if reading concurrently with writes
        return static_cast<DocId>(doc_id_to_string_.size());
    }

    // --- TagId Mapping Implementation ---

    TagId Mapping::getTagId(const StringTag& str_tag)
    {
        if (str_tag.empty())
        {
            return INVALID_TAG_ID;
        }
        // std::lock_guard<std::mutex> lock(map_mutex_);

        auto it = string_to_tag_id_.find(str_tag);
        if (it != string_to_tag_id_.end())
        {
            return it->second;
        }
        else
        {
            TagId new_id = static_cast<TagId>(tag_id_to_string_.size());
            if (new_id == INVALID_TAG_ID)
            {
                return INVALID_TAG_ID;
            }
            tag_id_to_string_.push_back(str_tag);
            string_to_tag_id_[str_tag] = new_id;
            return new_id;
        }
    }

    TagId Mapping::getTagId(StringTag&& str_tag)
    {
        if (str_tag.empty())
        {
            return INVALID_TAG_ID;
        }
        // std::lock_guard<std::mutex> lock(map_mutex_);

        auto it = string_to_tag_id_.find(str_tag);
        if (it != string_to_tag_id_.end())
        {
            return it->second;
        }
        else
        {
            TagId new_id = static_cast<TagId>(tag_id_to_string_.size());
            if (new_id == INVALID_TAG_ID)
            {
                return INVALID_TAG_ID;
            }
            tag_id_to_string_.push_back(std::move(str_tag));
            string_to_tag_id_[tag_id_to_string_.back()] = new_id; // Use string in vector as key
            return new_id;
        }
    }


    StringTag Mapping::getStringTag(TagId tag_id) const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_);

        if (tag_id < tag_id_to_string_.size())
        {
            return tag_id_to_string_[tag_id];
        }
        return "";
    }

    TagId Mapping::getNextTagId() const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_);
        return static_cast<TagId>(tag_id_to_string_.size());
    }

    // --- Utility Implementation ---

    size_t Mapping::getDocCount() const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_);
        return doc_id_to_string_.size();
    }

    size_t Mapping::getTagCount() const
    {
        // std::lock_guard<std::mutex> lock(map_mutex_);
        return tag_id_to_string_.size();
    }
} // namespace bitmap_index::core
