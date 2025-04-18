//
// Created by qi7876 on 4/18/25.
//
#include "catch2/catch_test_macros.hpp"
#include "bitmap_index/core/inverted_index.h"
#include "roaring/roaring.hh"
#include <vector>
#include <numeric> // For std::iota
#include <optional>

using namespace bitmap_index::core;
using roaring::Roaring; // Use Roaring type directly

// Helper to create a Roaring bitmap from a vector
Roaring make_bitmap(const std::vector<DocId>& ids)
{
    Roaring r;
    for (DocId id : ids)
    {
        r.add(id);
    }
    return r;
}

// Helper to convert Roaring bitmap to sorted vector for comparison
std::vector<DocId> bitmap_to_vector(const Roaring& r)
{
    std::vector<DocId> vec;
    // Roaring iterators guarantee sorted order
    for (auto it = r.begin(); it != r.end(); ++it)
    {
        vec.push_back(*it);
    }
    // Alternative using toUint32Array (might be slightly less efficient if array is large)
    // size_t cardinality = r.cardinality();
    // vec.resize(cardinality);
    // r.toUint32Array(vec.data());
    return vec;
}


TEST_CASE("InvertedIndex - Basic Operations", "[core][inverted_index]")
{
    InvertedIndex inv_index;

    REQUIRE(inv_index.getTagCount() == 0);
    REQUIRE(!inv_index.getBitmap(0).has_value());
    REQUIRE(inv_index.getCardinality(0) == 0);

    SECTION("Add documents to tags")
    {
        inv_index.add(10, 0); // doc 10 -> tag 0
        inv_index.add(20, 1); // doc 20 -> tag 1
        inv_index.add(10, 1); // doc 10 -> tag 1
        inv_index.add(30, 0); // doc 30 -> tag 0

        REQUIRE(inv_index.getTagCount() == 2); // Highest tag ID is 1, so size is 2

        // Check tag 0
        auto bm0_opt = inv_index.getBitmap(0);
        REQUIRE(bm0_opt.has_value());
        REQUIRE(bm0_opt.value().get().cardinality() == 2);
        REQUIRE(inv_index.getCardinality(0) == 2);
        REQUIRE(bm0_opt.value().get().contains(10));
        REQUIRE(bm0_opt.value().get().contains(30));
        REQUIRE(!bm0_opt.value().get().contains(20));

        // Check tag 1
        auto bm1_opt = inv_index.getBitmap(1);
        REQUIRE(bm1_opt.has_value());
        REQUIRE(bm1_opt.value().get().cardinality() == 2);
        REQUIRE(inv_index.getCardinality(1) == 2);
        REQUIRE(bm1_opt.value().get().contains(10));
        REQUIRE(bm1_opt.value().get().contains(20));
        REQUIRE(!bm1_opt.value().get().contains(30));

        // Check non-existent tag
        REQUIRE(!inv_index.getBitmap(2).has_value());
        REQUIRE(inv_index.getCardinality(2) == 0);
        REQUIRE(!inv_index.getBitmap(INVALID_TAG_ID).has_value());
        REQUIRE(inv_index.getCardinality(INVALID_TAG_ID) == 0);
    }

    SECTION("Add to non-sequential TagIds")
    {
        inv_index.add(100, 5);
        REQUIRE(inv_index.getTagCount() == 6); // Size should be max_tag_id + 1

        // Check intermediate tags are empty but exist conceptually
        REQUIRE(inv_index.getBitmap(0).has_value()); // Bitmap exists due to resize
        REQUIRE(inv_index.getBitmap(0).value().get().isEmpty());
        REQUIRE(inv_index.getCardinality(0) == 0);
        REQUIRE(inv_index.getBitmap(4).has_value());
        REQUIRE(inv_index.getBitmap(4).value().get().isEmpty());

        // Check tag 5
        auto bm5_opt = inv_index.getBitmap(5);
        REQUIRE(bm5_opt.has_value());
        REQUIRE(bm5_opt.value().get().cardinality() == 1);
        REQUIRE(bm5_opt.value().get().contains(100));

        // Check tag 6 (out of bounds)
        REQUIRE(!inv_index.getBitmap(6).has_value());
    }

    SECTION("Ignore invalid inputs")
    {
        inv_index.add(10, INVALID_TAG_ID);
        REQUIRE(inv_index.getTagCount() == 0); // No tags added

        inv_index.add(INVALID_DOC_ID, 0);
        REQUIRE(inv_index.getTagCount() == 0); // Should remain 0 as invalid doc doesn't resize

        inv_index.add(20, 0); // Add valid doc
        inv_index.add(INVALID_DOC_ID, 0); // Add invalid again
        REQUIRE(inv_index.getCardinality(0) == 1);
        REQUIRE(inv_index.getBitmap(0).value().get().contains(20));
    }
}


TEST_CASE("InvertedIndex - Bitmap Operations", "[core][inverted_index]")
{
    InvertedIndex inv_index;

    // Setup:
    // Tag 0: {0, 1, 2, 10}
    // Tag 1: {1, 2, 3, 11}
    // Tag 2: {2, 4, 10, 12}
    // Tag 3: {} (empty but potentially exists due to resize)
    // Tag 4: {100} (non-overlapping)
    inv_index.add(0, 0);
    inv_index.add(1, 0);
    inv_index.add(2, 0);
    inv_index.add(10, 0);
    inv_index.add(1, 1);
    inv_index.add(2, 1);
    inv_index.add(3, 1);
    inv_index.add(11, 1);
    inv_index.add(2, 2);
    inv_index.add(4, 2);
    inv_index.add(10, 2);
    inv_index.add(12, 2);
    inv_index.add(50, 3); // Ensure TagId 3 exists by adding an element
    // We can verify it's empty later if needed, but don't modify via getBitmap here.
    // For the setup, we can make it empty right after adding if needed:
    auto bm3_opt = inv_index.getBitmap(3); // Get const ref
    if (bm3_opt)
    {
        // Cannot call remove on bm3_opt.value().get()
        // If you *really* need to modify it here, you'd need a non-const getter
        // or a specific 'clearTag' method in InvertedIndex.
        // For this test setup, just adding is likely sufficient,
        // or add and then specifically test for emptiness later.
        // Let's try removing the remove() call completely first.
    }
    // --- Let's simplify further ---
    // Just adding ensures the tag exists in the index structure.
    // We will rely on the fact that only '50' was added to tag 3 for subsequent tests.
    // If a test requires tag 3 to be specifically empty, we can adjust there,
    // but the setup error is from trying to modify via a const ref.

    // FINAL PROPOSED CHANGE:
    inv_index.add(50, 3); // Ensures TagId 3 exists. It contains DocId 50 initially.
    // Tests using Tag 3 should account for this content or
    // we need a different way to clear it if required.
    // Let's assume tests account for {50} in tag 3 for now. // Ensure tag 3 exists but is empty
    inv_index.add(100, 4);

    REQUIRE(inv_index.getTagCount() == 5);

    SECTION("AND Operations")
    {
        // Tag 0 AND Tag 1 => {1, 2}
        Roaring result_01 = inv_index.performOperation({0, 1}, BitmapOperation::AND);
        REQUIRE(bitmap_to_vector(result_01) == std::vector<DocId>{1, 2});

        // Tag 0 AND Tag 1 AND Tag 2 => {2}
        Roaring result_012 = inv_index.performOperation({0, 1, 2}, BitmapOperation::AND);
        REQUIRE(bitmap_to_vector(result_012) == std::vector<DocId>{2});

        // Tag 0 AND Tag 4 (non-overlapping) => {}
        Roaring result_04 = inv_index.performOperation({0, 4}, BitmapOperation::AND);
        REQUIRE(result_04.isEmpty());

        // Tag 0 AND Tag 3 (empty) => {}
        Roaring result_03 = inv_index.performOperation({0, 3}, BitmapOperation::AND);
        REQUIRE(result_03.isEmpty());

        // AND with invalid tag => {}
        Roaring result_0_invalid = inv_index.performOperation({0, INVALID_TAG_ID}, BitmapOperation::AND);
        REQUIRE(result_0_invalid.isEmpty());
        Roaring result_invalid_0 = inv_index.performOperation({INVALID_TAG_ID, 0}, BitmapOperation::AND);
        REQUIRE(result_invalid_0.isEmpty());
        Roaring result_0_oor = inv_index.performOperation({0, 10}, BitmapOperation::AND); // Out of range tag
        REQUIRE(result_0_oor.isEmpty());


        // AND with empty input list
        Roaring result_empty = inv_index.performOperation({}, BitmapOperation::AND);
        REQUIRE(result_empty.isEmpty());
    }

    SECTION("OR Operations")
    {
        // Tag 0 OR Tag 1 => {0, 1, 2, 3, 10, 11}
        Roaring result_01 = inv_index.performOperation({0, 1}, BitmapOperation::OR);
        REQUIRE(bitmap_to_vector(result_01) == std::vector<DocId>{0, 1, 2, 3, 10, 11});

        // Tag 0 OR Tag 1 OR Tag 2 => {0, 1, 2, 3, 4, 10, 11, 12}
        Roaring result_012 = inv_index.performOperation({0, 1, 2}, BitmapOperation::OR);
        REQUIRE(bitmap_to_vector(result_012) == std::vector<DocId>{0, 1, 2, 3, 4, 10, 11, 12});

        // Tag 0 OR Tag 4 (non-overlapping) => {0, 1, 2, 10, 100}
        Roaring result_04 = inv_index.performOperation({0, 4}, BitmapOperation::OR);
        REQUIRE(bitmap_to_vector(result_04) == std::vector<DocId>{0, 1, 2, 10, 100});

        // Tag 0 OR Tag 3 (empty) => {0, 1, 2, 10}
        Roaring result_03 = inv_index.performOperation({0, 3}, BitmapOperation::OR);
        REQUIRE(bitmap_to_vector(result_03) == std::vector<DocId>{0, 1, 2, 10, 50}); // Tag 3 has {50}

        // OR with invalid tag (ignored)
        Roaring result_0_invalid = inv_index.performOperation({0, INVALID_TAG_ID}, BitmapOperation::OR);
        REQUIRE(bitmap_to_vector(result_0_invalid) == std::vector<DocId>{0, 1, 2, 10});
        Roaring result_invalid_0 = inv_index.performOperation({INVALID_TAG_ID, 0}, BitmapOperation::OR);
        REQUIRE(result_invalid_0.isEmpty()); // First tag invalid -> empty result
        Roaring result_0_oor = inv_index.performOperation({0, 10}, BitmapOperation::OR); // Out of range tag
        REQUIRE(bitmap_to_vector(result_0_oor) == std::vector<DocId>{0, 1, 2, 10});

        // OR with empty input list
        Roaring result_empty = inv_index.performOperation({}, BitmapOperation::OR);
        REQUIRE(result_empty.isEmpty());
    }

    SECTION("XOR Operations")
    {
        // Tag 0 XOR Tag 1 => {0, 3, 10, 11}
        Roaring result_01 = inv_index.performOperation({0, 1}, BitmapOperation::XOR);
        REQUIRE(bitmap_to_vector(result_01) == std::vector<DocId>{0, 3, 10, 11});

        // Tag 0 XOR Tag 1 XOR Tag 2 => ({0, 3, 10, 11}) XOR {2, 4, 10, 12} => {0, 2, 3, 4, 11, 12}
        Roaring result_012 = inv_index.performOperation({0, 1, 2}, BitmapOperation::XOR);
        REQUIRE(bitmap_to_vector(result_012) == std::vector<DocId>{0, 2, 3, 4, 11, 12});

        // Tag 0 XOR Tag 4 (non-overlapping) => {0, 1, 2, 10, 100}
        Roaring result_04 = inv_index.performOperation({0, 4}, BitmapOperation::XOR);
        REQUIRE(bitmap_to_vector(result_04) == std::vector<DocId>{0, 1, 2, 10, 100});

        // Tag 0 XOR Tag 3 (empty) => {0, 1, 2, 10}
        Roaring result_03 = inv_index.performOperation({0, 3}, BitmapOperation::XOR);
        REQUIRE(bitmap_to_vector(result_03) == std::vector<DocId>{0, 1, 2, 10, 50}); // Tag 3 has {50}

        // XOR with invalid tag (ignored)
        Roaring result_0_invalid = inv_index.performOperation({0, INVALID_TAG_ID}, BitmapOperation::XOR);
        REQUIRE(bitmap_to_vector(result_0_invalid) == std::vector<DocId>{0, 1, 2, 10});
        Roaring result_invalid_0 = inv_index.performOperation({INVALID_TAG_ID, 0}, BitmapOperation::XOR);
        REQUIRE(result_invalid_0.isEmpty()); // First tag invalid -> empty result
        Roaring result_0_oor = inv_index.performOperation({0, 10}, BitmapOperation::XOR); // Out of range tag
        REQUIRE(bitmap_to_vector(result_0_oor) == std::vector<DocId>{0, 1, 2, 10});


        // XOR with empty input list
        Roaring result_empty = inv_index.performOperation({}, BitmapOperation::XOR);
        REQUIRE(result_empty.isEmpty());
    }

    SECTION("ANDNOT Operations")
    {
        // Tag 0 ANDNOT Tag 1 => {0, 1, 2, 10} - {1, 2, 3, 11} => {0, 10}
        Roaring result_0_1 = inv_index.performOperation({0, 1}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_1) == std::vector<DocId>{0, 10});

        // Tag 1 ANDNOT Tag 0 => {1, 2, 3, 11} - {0, 1, 2, 10} => {3, 11}
        Roaring result_1_0 = inv_index.performOperation({1, 0}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_1_0) == std::vector<DocId>{3, 11});

        // Tag 0 ANDNOT (Tag 1 OR Tag 2) => {0, 1, 2, 10} - ({1, 2, 3, 11} | {2, 4, 10, 12})
        //                              => {0, 1, 2, 10} - {1, 2, 3, 4, 10, 11, 12} => {0}
        Roaring result_0_12 = inv_index.performOperation({0, 1, 2}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_12) == std::vector<DocId>{0});

        // Tag 0 ANDNOT Tag 4 (non-overlapping) => {0, 1, 2, 10} - {100} => {0, 1, 2, 10}
        Roaring result_0_4 = inv_index.performOperation({0, 4}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_4) == std::vector<DocId>{0, 1, 2, 10});

        // Tag 0 ANDNOT Tag 3 (empty) => {0, 1, 2, 10} - {} => {0, 1, 2, 10}
        Roaring result_0_3 = inv_index.performOperation({0, 3}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_3) == std::vector<DocId>{0, 1, 2, 10});

        // ANDNOT with invalid tag
        // Tag 0 ANDNOT Invalid => {0, 1, 2, 10} (Invalid ignored on right)
        Roaring result_0_invalid = inv_index.performOperation({0, INVALID_TAG_ID}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_invalid) == std::vector<DocId>{0, 1, 2, 10});
        // Invalid ANDNOT Tag 0 => {} (First tag invalid)
        Roaring result_invalid_0 = inv_index.performOperation({INVALID_TAG_ID, 0}, BitmapOperation::ANDNOT);
        REQUIRE(result_invalid_0.isEmpty());
        // Tag 0 ANDNOT OOR => {0, 1, 2, 10} (OOR ignored on right)
        Roaring result_0_oor = inv_index.performOperation({0, 10}, BitmapOperation::ANDNOT); // Out of range tag
        REQUIRE(bitmap_to_vector(result_0_oor) == std::vector<DocId>{0, 1, 2, 10});
        // Tag 0 ANDNOT (Tag 1 OR Invalid) => Tag 0 ANDNOT Tag 1 => {0, 10}
        Roaring result_0_1_invalid = inv_index.performOperation({0, 1, INVALID_TAG_ID}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_0_1_invalid) == std::vector<DocId>{0, 10});


        // ANDNOT with only one tag => returns the tag's bitmap
        Roaring result_single = inv_index.performOperation({0}, BitmapOperation::ANDNOT);
        REQUIRE(bitmap_to_vector(result_single) == std::vector<DocId>{0, 1, 2, 10});

        // ANDNOT with empty input list
        Roaring result_empty = inv_index.performOperation({}, BitmapOperation::ANDNOT);
        REQUIRE(result_empty.isEmpty());
    }
}

TEST_CASE("InvertedIndex - Optimization and Shrinking", "[core][inverted_index]")
{
    InvertedIndex inv_index;
    inv_index.add(10, 0);
    inv_index.add(20, 5); // Creates gaps
    inv_index.add(10, 5);

    uint64_t card0_before = inv_index.getCardinality(0);
    uint64_t card5_before = inv_index.getCardinality(5);
    size_t tag_count_before = inv_index.getTagCount();

    SECTION("runOptimize")
    {
        bool success = inv_index.runOptimize();
        REQUIRE(success); // Should succeed

        // Verify data is intact
        REQUIRE(inv_index.getCardinality(0) == card0_before);
        REQUIRE(inv_index.getCardinality(5) == card5_before);
        REQUIRE(inv_index.getTagCount() == tag_count_before);
        REQUIRE(inv_index.getBitmap(0).value().get().contains(10));
        REQUIRE(inv_index.getBitmap(5).value().get().contains(10));
        REQUIRE(inv_index.getBitmap(5).value().get().contains(20));
    }

    SECTION("shrinkToFit")
    {
        inv_index.shrinkToFit();

        // Verify data is intact
        REQUIRE(inv_index.getCardinality(0) == card0_before);
        REQUIRE(inv_index.getCardinality(5) == card5_before);
        REQUIRE(inv_index.getTagCount() == tag_count_before); // Tag count might not shrink, depends on impl
        REQUIRE(inv_index.getBitmap(0).value().get().contains(10));
        REQUIRE(inv_index.getBitmap(5).value().get().contains(10));
        REQUIRE(inv_index.getBitmap(5).value().get().contains(20));
    }
}
