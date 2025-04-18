//
// Created by qi7876 on 4/18/25.
//
#include "catch2/catch_test_macros.hpp"
#include "bitmap_index/core/forward_index.h"
#include <vector>
#include <algorithm> // For std::sort
#include <numeric>   // For std::iota

using namespace bitmap_index::core;

// Helper function to compare vectors regardless of order
bool compareVectorsUnordered(std::vector<TagId> v1, std::vector<TagId> v2)
{
    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());
    return v1 == v2;
}

TEST_CASE("ForwardIndex - Basic Operations", "[core][forward_index]")
{
    ForwardIndex fwd_index;

    REQUIRE(fwd_index.getDocCount() == 0);
    REQUIRE(fwd_index.getTags(0).empty());
    REQUIRE(fwd_index.getTags(INVALID_DOC_ID).empty());

    SECTION("Add tags using addTags (const ref)")
    {
        const std::vector<TagId> tags1 = {1, 3, 2};
        const std::vector<TagId> tags2 = {5};

        fwd_index.addTags(0, tags1);
        fwd_index.addTags(1, tags2);

        REQUIRE(fwd_index.getDocCount() == 2);
        // Use unordered comparison since internal order might not be guaranteed later
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(0), tags1));
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(1), tags2));

        // Overwrite tags for doc 0
        const std::vector<TagId> tags1_new = {8, 9};
        fwd_index.addTags(0, tags1_new);
        REQUIRE(fwd_index.getDocCount() == 2); // Count shouldn't change
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(0), tags1_new));
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(1), tags2)); // Doc 1 unchanged
    }

    SECTION("Add tags using addTags (rvalue ref)")
    {
        std::vector<TagId> tags1 = {10, 30, 20};
        std::vector<TagId> tags1_copy = tags1; // Copy for comparison after move

        fwd_index.addTags(0, std::move(tags1));

        REQUIRE(fwd_index.getDocCount() == 1);
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(0), tags1_copy));
        // tags1 is now in a moved-from state
    }

    SECTION("Add tags incrementally using addTag")
    {
        fwd_index.addTag(0, 10);
        fwd_index.addTag(1, 20);
        fwd_index.addTag(0, 15);
        fwd_index.addTag(1, 25);
        fwd_index.addTag(0, 10); // Add duplicate

        REQUIRE(fwd_index.getDocCount() == 2);

        std::vector<TagId> expected_tags0 = {10, 15, 10}; // Order preserved by push_back
        std::vector<TagId> expected_tags1 = {20, 25};
        REQUIRE(fwd_index.getTags(0) == expected_tags0);
        REQUIRE(fwd_index.getTags(1) == expected_tags1);

        // Verify with unordered compare as well
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(0), {10, 15, 10}));
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(1), {20, 25}));
    }

    SECTION("Add tags to non-sequential DocIds")
    {
        fwd_index.addTag(2, 50);
        REQUIRE(fwd_index.getDocCount() == 3);
        REQUIRE(fwd_index.getTags(0).empty());
        REQUIRE(fwd_index.getTags(1).empty());
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(2), {50}));

        fwd_index.addTags(0, {1, 2});
        REQUIRE(fwd_index.getDocCount() == 3);
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(0), {1, 2}));
        REQUIRE(fwd_index.getTags(1).empty());
        REQUIRE(compareVectorsUnordered(fwd_index.getTags(2), {50}));
    }

    SECTION("Retrieve tags for out-of-bounds or invalid DocIds")
    {
        fwd_index.addTag(0, 1);
        fwd_index.addTag(1, 2);

        REQUIRE(fwd_index.getTags(2).empty()); // ID just beyond current max
        REQUIRE(fwd_index.getTags(100).empty()); // Far beyond
        REQUIRE(fwd_index.getTags(INVALID_DOC_ID).empty());
        REQUIRE(fwd_index.getTags(0) == std::vector<TagId>{1}); // Verify existing is fine
    }

    SECTION("Ignore invalid inputs")
    {
        fwd_index.addTag(INVALID_DOC_ID, 5);
        REQUIRE(fwd_index.getDocCount() == 0); // Should not resize or add

        fwd_index.addTag(0, INVALID_TAG_ID);
        REQUIRE(fwd_index.getDocCount() == 0 ); // Should remain 0 as invalid tag doesn't resize
        REQUIRE(fwd_index.getTags(0).empty()); // But invalid tag wasn't added

        fwd_index.addTag(0, 10); // Add valid tag
        fwd_index.addTag(INVALID_DOC_ID, 15); // Add another invalid
        REQUIRE(fwd_index.getTags(0) == std::vector<TagId>{10}); // Should only contain the valid tag

        std::vector<TagId> tags_with_invalid = {1, INVALID_TAG_ID, 3};
        // Note: Current addTags implementation doesn't filter invalid IDs within the vector.
        // If filtering is desired, it needs to be added to addTags.
        fwd_index.addTags(1, tags_with_invalid);
        REQUIRE(fwd_index.getTags(1) == tags_with_invalid);
    }
}
