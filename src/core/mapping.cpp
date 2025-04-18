#include "bitmap_index/core/mapping.h"
#include <utility> // For std::move
#include <iosfwd> // For std::ostream, std::istream
#include <vector>
#include <fstream> // Include fstream for ofstream/ifstream operations
#include <iostream> // For error logging


namespace bitmap_index::core
{
    // --- Basic Binary I/O Helpers (Simplified - Repeated for Scoping) ---
    // Ideally place these in a shared utility header/cpp
    template<typename T>
    inline bool write_binary(std::ostream& os, const T& value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(T));
        return os.good();
    }
    template<typename T>
    inline bool read_binary(std::istream& is, T& value) {
        is.read(reinterpret_cast<char*>(&value), sizeof(T));
        return is.good() && (is.gcount() == sizeof(T));
    }
    // Helper to write a string (size first, then data)
    inline bool write_string(std::ostream& os, const std::string& str) {
        uint64_t len = str.length();
        if (!write_binary(os, len)) return false;
        os.write(str.data(), len);
        return os.good();
    }
    // Helper to read a string (size first, then data)
    inline bool read_string(std::istream& is, std::string& str) {
        uint64_t len = 0;
        if (!read_binary(is, len)) return false;
        // Resize string and read directly into its buffer (requires C++11/17+)
        str.resize(len);
        // Check for len > 0 before reading to avoid issues with some stream implementations
        if (len > 0) {
            is.read(&str[0], len); // Read into the string's buffer
            return is.good() && (static_cast<uint64_t>(is.gcount()) == len);
        }
        return is.good(); // Reading 0 bytes is okay
    }

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

    void Mapping::clear() {
        doc_id_to_string_.clear();
        tag_id_to_string_.clear();
        string_to_doc_id_.clear();
        string_to_tag_id_.clear();

        // Optional: reclaim memory if needed
        // doc_id_to_string_.shrink_to_fit();
        // tag_id_to_string_.shrink_to_fit();
    }
    bool Mapping::save(std::ostream& os) const {
        // std::shared_lock lock(map_mutex_); // Read lock

        try {
            // Save Doc ID mapping vector
            uint64_t doc_vec_size = doc_id_to_string_.size();
            if (!write_binary(os, doc_vec_size)) return false;
            for (const auto& str : doc_id_to_string_) {
                if (!write_string(os, str)) return false;
            }

            // Save Tag ID mapping vector
            uint64_t tag_vec_size = tag_id_to_string_.size();
            if (!write_binary(os, tag_vec_size)) return false;
            for (const auto& str : tag_id_to_string_) {
                if (!write_string(os, str)) return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error saving Mapping: " << e.what() << std::endl;
            return false;
        }
        return os.good();
    }


    bool Mapping::load(std::istream& is) {
        // std::unique_lock lock(map_mutex_); // Write lock
        clear(); // Start fresh

        try {
            // Load Doc ID mapping vector
            uint64_t doc_vec_size = 0;
            if (!read_binary(is, doc_vec_size)) {
                 if (is.eof() && is.gcount() == 0) return true; // Empty file OK
                 std::cerr << "Error reading doc vector size." << std::endl;
                 return false;
            }
            doc_id_to_string_.resize(static_cast<size_t>(doc_vec_size));
            string_to_doc_id_.reserve(static_cast<size_t>(doc_vec_size)); // Pre-allocate map
            for (uint64_t i = 0; i < doc_vec_size; ++i) {
                if (!read_string(is, doc_id_to_string_[i])) {
                    std::cerr << "Error reading doc string " << i << std::endl;
                    return false;
                }
                // Rebuild reverse map
                string_to_doc_id_[doc_id_to_string_[i]] = static_cast<DocId>(i);
            }

            // Load Tag ID mapping vector
            uint64_t tag_vec_size = 0;
            if (!read_binary(is, tag_vec_size)) {
                 std::cerr << "Error reading tag vector size." << std::endl;
                 return false; // Should have tag data if doc data was present
            }
             tag_id_to_string_.resize(static_cast<size_t>(tag_vec_size));
             string_to_tag_id_.reserve(static_cast<size_t>(tag_vec_size)); // Pre-allocate map
             for (uint64_t i = 0; i < tag_vec_size; ++i) {
                 if (!read_string(is, tag_id_to_string_[i])) {
                      std::cerr << "Error reading tag string " << i << std::endl;
                      return false;
                 }
                 // Rebuild reverse map
                 string_to_tag_id_[tag_id_to_string_[i]] = static_cast<TagId>(i);
             }
        } catch (const std::exception& e) {
            std::cerr << "Error loading Mapping: " << e.what() << std::endl;
            clear(); // Clear potentially partial load
            return false;
        }
        return is.good() || is.eof();
    }
} // namespace bitmap_index::core
