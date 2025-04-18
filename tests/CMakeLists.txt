# Find Catch2 (vcpkg should make this available)
find_package(Catch2 CONFIG REQUIRED)

# --- Test String Utils ---
add_executable(test_utils_string utils/test_string_util.cpp)
# Link against our library and Catch2's main function provider
target_link_libraries(test_utils_string PRIVATE bitmap_index Catch2::Catch2WithMain)
# Add to CTest
include(CTest)
add_test(NAME Utils.String COMMAND test_utils_string)

# --- Test File Utils ---
add_executable(test_utils_file utils/test_file_util.cpp)
# Link against our library and Catch2's main function provider
# Need to link filesystem for C++17 filesystem features if not automatically handled
# Modern CMake often handles this via target_link_libraries with C++ standard setting,
# but explicitly linking can be necessary sometimes. Check if needed.
# target_link_libraries(test_utils_file PRIVATE std::filesystem) # Usually not needed if CXX_STANDARD >= 17
target_link_libraries(test_utils_file PRIVATE bitmap_index Catch2::Catch2WithMain)
# Add to CTest
add_test(NAME Utils.File COMMAND test_utils_file)

# --- Test File Utils ---
add_executable(test_io_csv_parser io/test_csv_parser.cpp)
# Link against our library and Catch2's main function provider
# Need to link filesystem for C++17 filesystem features if not automatically handled
# Modern CMake often handles this via target_link_libraries with C++ standard setting,
# but explicitly linking can be necessary sometimes. Check if needed.
# target_link_libraries(test_utils_file PRIVATE std::filesystem) # Usually not needed if CXX_STANDARD >= 17
target_link_libraries(test_io_csv_parser PRIVATE bitmap_index Catch2::Catch2WithMain)
# Add to CTest
add_test(NAME IO.CSVParser COMMAND test_io_csv_parser)

# --- Test File Utils ---
add_executable(test_core_mapping core/test_mapping.cpp)
# Link against our library and Catch2's main function provider
# Need to link filesystem for C++17 filesystem features if not automatically handled
# Modern CMake often handles this via target_link_libraries with C++ standard setting,
# but explicitly linking can be necessary sometimes. Check if needed.
# target_link_libraries(test_utils_file PRIVATE std::filesystem) # Usually not needed if CXX_STANDARD >= 17
target_link_libraries(test_core_mapping PRIVATE bitmap_index Catch2::Catch2WithMain)
# Add to CTest
add_test(NAME CORE.Mapping COMMAND test_core_mapping)

# Add more tests here following the same pattern...