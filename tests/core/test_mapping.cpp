#include "catch2/catch_test_macros.hpp"
#include "bitmap_index/core/mapping.h"
#include "bitmap_index/core/types.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>

namespace fs = std::filesystem;
using namespace bitmap_index::core;


TEST_CASE("Mapping - Basic Document ID Operations", "[core][mapping]") {
    Mapping mapping;

    REQUIRE(mapping.getNextDocId() == 0);
    REQUIRE(mapping.getDocCount() == 0);

    SECTION("Add and retrieve first document ID") {
        StringId id_str1 = "doc001";
        DocId id1 = mapping.getId(id_str1);
        REQUIRE(id1 == 0);
        REQUIRE(mapping.getNextDocId() == 1);
        REQUIRE(mapping.getDocCount() == 1);
        REQUIRE(mapping.getStringId(id1) == id_str1);
    }

    SECTION("Add the same document ID again") {
        StringId id_str1 = "doc001";
        DocId id1 = mapping.getId(id_str1); // First add
        REQUIRE(id1 == 0);

        DocId id1_again = mapping.getId(id_str1); // Add same again
        REQUIRE(id1_again == id1); // Should return the same ID
        REQUIRE(mapping.getNextDocId() == 1); // Next ID should not change
        REQUIRE(mapping.getDocCount() == 1); // Count should not change
    }

    SECTION("Add multiple unique document IDs") {
        StringId id_str1 = "doc_A";
        StringId id_str2 = "doc_B";
        StringId id_str3 = "doc_C";

        DocId id1 = mapping.getId(id_str1);
        DocId id2 = mapping.getId(id_str2);
        DocId id3 = mapping.getId(id_str3);

        REQUIRE(id1 == 0);
        REQUIRE(id2 == 1);
        REQUIRE(id3 == 2);
        REQUIRE(mapping.getNextDocId() == 3);
        REQUIRE(mapping.getDocCount() == 3);

        // Verify retrieval
        REQUIRE(mapping.getStringId(id1) == id_str1);
        REQUIRE(mapping.getStringId(id2) == id_str2);
        REQUIRE(mapping.getStringId(id3) == id_str3);
    }

    SECTION("Retrieve non-existent or invalid document IDs") {
        REQUIRE(mapping.getStringId(0) == ""); // No IDs added yet
        REQUIRE(mapping.getStringId(100) == "");
        REQUIRE(mapping.getStringId(INVALID_DOC_ID) == "");

        // Add one ID
        mapping.getId("doc_X");
        REQUIRE(mapping.getStringId(1) == ""); // ID 1 doesn't exist yet
    }

     SECTION("Handle empty string document ID input") {
        StringId empty_str = "";
        DocId invalid_id = mapping.getId(empty_str);
        REQUIRE(invalid_id == INVALID_DOC_ID);
        REQUIRE(mapping.getNextDocId() == 0); // No ID should be assigned
        REQUIRE(mapping.getDocCount() == 0);
    }
}

TEST_CASE("Mapping - Basic Tag ID Operations", "[core][mapping]") {
    Mapping mapping;

    REQUIRE(mapping.getNextTagId() == 0);
    REQUIRE(mapping.getTagCount() == 0);

    SECTION("Add and retrieve first tag ID") {
        StringTag tag_str1 = "category:sports";
        TagId id1 = mapping.getTagId(tag_str1);
        REQUIRE(id1 == 0);
        REQUIRE(mapping.getNextTagId() == 1);
        REQUIRE(mapping.getTagCount() == 1);
        REQUIRE(mapping.getStringTag(id1) == tag_str1);
    }

    SECTION("Add the same tag ID again") {
        StringTag tag_str1 = "category:sports";
        TagId id1 = mapping.getTagId(tag_str1); // First add
        REQUIRE(id1 == 0);

        TagId id1_again = mapping.getTagId(tag_str1); // Add same again
        REQUIRE(id1_again == id1); // Should return the same ID
        REQUIRE(mapping.getNextTagId() == 1); // Next ID should not change
        REQUIRE(mapping.getTagCount() == 1); // Count should not change
    }

    SECTION("Add multiple unique tag IDs") {
        StringTag tag_str1 = "color:red";
        StringTag tag_str2 = "size:large";
        StringTag tag_str3 = "material:cotton";

        TagId id1 = mapping.getTagId(tag_str1);
        TagId id2 = mapping.getTagId(tag_str2);
        TagId id3 = mapping.getTagId(tag_str3);

        REQUIRE(id1 == 0);
        REQUIRE(id2 == 1);
        REQUIRE(id3 == 2);
        REQUIRE(mapping.getNextTagId() == 3);
        REQUIRE(mapping.getTagCount() == 3);

        // Verify retrieval
        REQUIRE(mapping.getStringTag(id1) == tag_str1);
        REQUIRE(mapping.getStringTag(id2) == tag_str2);
        REQUIRE(mapping.getStringTag(id3) == tag_str3);
    }

     SECTION("Retrieve non-existent or invalid tag IDs") {
        REQUIRE(mapping.getStringTag(0) == ""); // No IDs added yet
        REQUIRE(mapping.getStringTag(100) == "");
        REQUIRE(mapping.getStringTag(INVALID_TAG_ID) == "");

        // Add one ID
        mapping.getTagId("tag_X");
        REQUIRE(mapping.getStringTag(1) == ""); // ID 1 doesn't exist yet
    }

     SECTION("Handle empty string tag ID input") {
        StringTag empty_str = "";
        TagId invalid_id = mapping.getTagId(empty_str);
        REQUIRE(invalid_id == INVALID_TAG_ID);
        REQUIRE(mapping.getNextTagId() == 0); // No ID should be assigned
        REQUIRE(mapping.getTagCount() == 0);
    }
}

TEST_CASE("Mapping - Move Semantics", "[core][mapping]") {
    Mapping mapping;

    SECTION("Move semantics for DocId") {
        StringId original_doc_str = "move_doc_1";
        std::string copy_doc_str = original_doc_str; // Keep a copy for verification

        DocId id1 = mapping.getId(std::move(original_doc_str));
        // After move, original_doc_str is in a valid but unspecified state.
        // Don't rely on its value, but test the mapping.

        REQUIRE(id1 == 0);
        REQUIRE(mapping.getNextDocId() == 1);
        REQUIRE(mapping.getDocCount() == 1);
        REQUIRE(mapping.getStringId(id1) == copy_doc_str); // Verify using the copied value

        // Add another one using move
        StringId original_doc_str2 = "move_doc_2";
         std::string copy_doc_str2 = original_doc_str2;
         DocId id2 = mapping.getId(std::move(original_doc_str2));
         REQUIRE(id2 == 1);
         REQUIRE(mapping.getNextDocId() == 2);
         REQUIRE(mapping.getDocCount() == 2);
         REQUIRE(mapping.getStringId(id2) == copy_doc_str2);

         // Check that getting the moved ID again works
         DocId id1_again = mapping.getId(copy_doc_str); // Use the copy to lookup
         REQUIRE(id1_again == id1);

    }

     SECTION("Move semantics for TagId") {
        StringTag original_tag_str = "move_tag_A";
        std::string copy_tag_str = original_tag_str; // Keep a copy

        TagId id1 = mapping.getTagId(std::move(original_tag_str));

        REQUIRE(id1 == 0);
        REQUIRE(mapping.getNextTagId() == 1);
        REQUIRE(mapping.getTagCount() == 1);
        REQUIRE(mapping.getStringTag(id1) == copy_tag_str);

         // Add another one using move
        StringTag original_tag_str2 = "move_tag_B";
         std::string copy_tag_str2 = original_tag_str2;
         TagId id2 = mapping.getTagId(std::move(original_tag_str2));
         REQUIRE(id2 == 1);
         REQUIRE(mapping.getNextTagId() == 2);
         REQUIRE(mapping.getTagCount() == 2);
         REQUIRE(mapping.getStringTag(id2) == copy_tag_str2);

         // Check that getting the moved ID again works
         TagId id1_again = mapping.getTagId(copy_tag_str); // Use the copy to lookup
         REQUIRE(id1_again == id1);
    }
}