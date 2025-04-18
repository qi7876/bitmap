#include "bitmap_index/core/inverted_index.h"
#include <stdexcept> // For potential future exceptions
#include <limits>    // For numeric_limits

namespace bitmap_index::core {

// Helper function to ensure vector size (internal use)
bool InvertedIndex::ensureTagCapacity(const TagId tag_id) {
     if (tag_id == INVALID_TAG_ID) {
        // Log error or handle appropriately
        return false;
    }
    if (tag_id >= tag_to_bitmap_.size()) {
        // Resize, default constructing Roaring bitmaps for new elements
        // Ensure size doesn't exceed potential limits if necessary
        const size_t new_size = static_cast<size_t>(tag_id) + 1;
        // Add basic check, though vector might throw bad_alloc earlier
        if (new_size > tag_to_bitmap_.max_size()) {
             // Log error: Requested size exceeds vector capacity limit
             return false;
        }
        try {
            tag_to_bitmap_.resize(new_size);
        } catch (const std::bad_alloc& e) {
            // Log error: Failed to allocate memory for resizing
            return false;
        } catch (const std::length_error& e) {
             // Log error: Length error during resize
             return false;
        }
    }
    return true;
}


void InvertedIndex::add(DocId doc_id, TagId tag_id) {
    // std::unique_lock lock(rw_mutex_); // Write lock if concurrent

    if (doc_id == INVALID_DOC_ID) return; // Or log/throw

    // Ensure the vector is large enough *before* accessing the element
    if (!ensureTagCapacity(tag_id)) {
        // Handle error: could not ensure capacity (e.g., invalid tag_id or allocation failed)
        // Log::Error("Failed to ensure capacity for tag_id: {}", tag_id);
        return;
    }

    // Add the document ID to the corresponding bitmap
    try {
        tag_to_bitmap_[tag_id].add(doc_id);
    } catch (const std::exception& e) {
         // Catch potential exceptions from Roaring's add (though typically safe)
         // Log::Error("Exception adding doc_id {} to tag_id {}: {}", doc_id, tag_id, e.what());
    }
}

std::optional<std::reference_wrapper<const roaring::Roaring>> InvertedIndex::getBitmap(TagId tag_id) const {
    // std::shared_lock lock(rw_mutex_); // Read lock if concurrent

    if (tag_id == INVALID_TAG_ID || tag_id >= tag_to_bitmap_.size()) {
        return std::nullopt; // Tag ID is out of bounds
    }

    // Check if the bitmap for this tag actually contains anything.
    // Roaring bitmaps are default-constructed, so they might exist but be empty.
    // Depending on desired semantics, you might return nullopt only if tag_id >= size,
    // or also if the bitmap at tag_id exists but is empty. Let's return if it exists.
    // if (tag_to_bitmap_[tag_id].isEmpty()) {
    //     return std::nullopt; // No documents associated with this tag
    // }

    return std::cref(tag_to_bitmap_[tag_id]);
}

roaring::Roaring InvertedIndex::performOperation(const std::vector<TagId>& tag_ids, BitmapOperation op) const {
    // std::shared_lock lock(rw_mutex_); // Read lock if concurrent

    if (tag_ids.empty()) {
        return {}; // Return empty bitmap if no tags provided
    }

    // Retrieve the first valid bitmap
    const auto first_bitmap_opt = getBitmap(tag_ids[0]);
    if (!first_bitmap_opt) {
         // First tag is invalid or has no bitmap, result is empty for AND/ANDNOT
         // For OR/XOR, we could continue, but let's return empty for simplicity/consistency here.
         // More nuanced handling might be needed based on exact requirements.
        return {};
    }

    // Start with a copy of the first bitmap
    roaring::Roaring result = first_bitmap_opt.value().get();

    // Process remaining tags
    for (size_t i = 1; i < tag_ids.size(); ++i) {
        auto next_bitmap_opt = getBitmap(tag_ids[i]);
        if (!next_bitmap_opt) {
            // Handle invalid subsequent tag ID based on operation
            if (op == BitmapOperation::AND) {
                return {}; // Intersection with nothing is empty
            } else if (op == BitmapOperation::ANDNOT) {
                // A ANDNOT (B OR C OR ... Invalid ...) - invalid doesn't change the right side
                 continue; // Skip invalid tag for ANDNOT right side
            } else {
                // For OR and XOR, operating with an empty set doesn't change the result
                continue;
            }
        }

        const roaring::Roaring& next_bitmap = next_bitmap_opt.value().get();

        try {
            switch (op) {
                case BitmapOperation::AND:
                    result &= next_bitmap; // In-place AND
                    break;
                case BitmapOperation::OR:
                    result |= next_bitmap; // In-place OR
                    break;
                case BitmapOperation::XOR:
                    result ^= next_bitmap; // In-place XOR
                    break;
                case BitmapOperation::ANDNOT:
                    // For ANDNOT, the logic is result = result ANDNOT (bitmap[1] OR bitmap[2] ...)
                    // This requires accumulating the OR of the right-hand side first.
                    // Let's re-implement this part specifically for ANDNOT.
                    // The initial loop structure assumed pairwise A op B op C...
                    goto handle_andnot; // Jump to specific ANDNOT logic
            }
        } catch (const std::exception& e) {
             // Log error performing operation
             // Log::Error("Exception during bitmap operation: {}", e.what());
             return {}; // Return empty on error
        }
         // Optimization: if result becomes empty during AND, can stop early
         if (op == BitmapOperation::AND && result.isEmpty()) {
             return {};
         }

    } // End for loop

    // If operation was ANDNOT, the loop above was skipped for i > 0
    if (op == BitmapOperation::ANDNOT) {
handle_andnot:
        if (tag_ids.size() > 1) {
            // Compute the OR of all bitmaps from the second one onwards
            roaring::Roaring right_side;
            for (size_t i = 1; i < tag_ids.size(); ++i) {
                auto next_bitmap_opt = getBitmap(tag_ids[i]);
                if (next_bitmap_opt) { // Only OR valid bitmaps
                    try {
                        right_side |= next_bitmap_opt.value().get();
                    } catch (const std::exception& e) {
                        // Log error
                        return roaring::Roaring();
                    }
                }
            }
            // Now perform the ANDNOT operation: result = first_bitmap ANDNOT right_side
             try {
                result -= right_side; // In-place ANDNOT (operator-=)
             } catch (const std::exception& e) {
                  // Log error
                  return roaring::Roaring();
             }
        }
        // If only one tag_id for ANDNOT, result is just the first bitmap (already set)
    }


    return result;
}


uint64_t InvertedIndex::getCardinality(TagId tag_id) const {
    // std::shared_lock lock(rw_mutex_); // Read lock if concurrent
     auto bitmap_opt = getBitmap(tag_id);
     if (bitmap_opt) {
         try {
            return bitmap_opt.value().get().cardinality();
         } catch (const std::exception& e) {
              // Log error getting cardinality
              return 0;
         }
     }
     return 0; // Tag not found or invalid
}

size_t InvertedIndex::getTagCount() const {
    // std::shared_lock lock(rw_mutex_); // Read lock if concurrent
    return tag_to_bitmap_.size();
}

bool InvertedIndex::runOptimize() {
    // std::unique_lock lock(rw_mutex_); // Write lock - runOptimize modifies bitmaps

    bool success = true;
    for (auto& bitmap : tag_to_bitmap_) {
        // runOptimize returns true if layout changed. We just care if it ran without error.
        try {
             if (!bitmap.isEmpty()) { // Only optimize non-empty bitmaps
                bitmap.runOptimize();
             }
        } catch (const std::exception& e) {
             // Log error optimizing bitmap
             success = false;
             // Decide whether to continue optimizing others or stop
        }
    }
    return success;
}

void InvertedIndex::shrinkToFit() {
     // std::unique_lock lock(rw_mutex_); // Write lock - shrinkToFit modifies bitmaps

     for (auto& bitmap : tag_to_bitmap_) {
          try {
            bitmap.shrinkToFit();
          } catch (const std::exception& e) {
               // Log error shrinking bitmap
               // Decide whether to continue or stop
          }
     }
     // Also shrink the main vector itself
     try {
        tag_to_bitmap_.shrink_to_fit();
     } catch (const std::exception& e) {
          // Log error shrinking vector
     }
}


} // namespace bitmap_index::core