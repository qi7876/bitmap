# Find Catch2 (vcpkg should make this available)
find_package(Catch2 CONFIG REQUIRED)

# --- Test String Utils ---
add_executable(test_utils_string utils/test_string_util.cpp)
# Link against our library and Catch2's main function provider
target_link_libraries(test_utils_string PRIVATE bitmap_index Catch2::Catch2WithMain)
include(CTest)
add_test(NAME Utils.String COMMAND test_utils_string)


add_executable(test_utils_file utils/test_file_util.cpp)
target_link_libraries(test_utils_file PRIVATE bitmap_index Catch2::Catch2WithMain)
add_test(NAME Utils.File COMMAND test_utils_file)

add_executable(test_io_csv_parser io/test_csv_parser.cpp)
target_link_libraries(test_io_csv_parser PRIVATE bitmap_index Catch2::Catch2WithMain)
add_test(NAME IO.CSVParser COMMAND test_io_csv_parser)

add_executable(test_core_mapping core/test_mapping.cpp)
target_link_libraries(test_core_mapping PRIVATE bitmap_index Catch2::Catch2WithMain)
add_test(NAME CORE.Mapping COMMAND test_core_mapping)

add_executable(test_core_forward_index core/test_forward_index.cpp)
target_link_libraries(test_core_forward_index PRIVATE bitmap_index Catch2::Catch2WithMain)
add_test(NAME CORE.ForwardIndex COMMAND test_core_forward_index)

add_executable(test_core_inverted_index core/test_inverted_index.cpp)
target_link_libraries(test_core_inverted_index PRIVATE bitmap_index Catch2::Catch2WithMain)
add_test(NAME CORE.InvertedIndex COMMAND test_core_inverted_index)