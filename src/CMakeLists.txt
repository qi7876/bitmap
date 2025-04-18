# 查找 Roaring 库 (vcpkg 会处理好)
find_package(roaring CONFIG REQUIRED)

# 定义库目标 (静态库或动态库)
add_library(bitmap_index STATIC # 或者 SHARED
    core/index_manager.cpp
    core/forward_index.cpp
    core/inverted_index.cpp
    core/mapping.cpp
    io/csv_parser.cpp
    utils/file_util.cpp
    utils/string_util.cpp
    # ... 其他源文件
)

# 指定包含目录 (公共头文件)
target_include_directories(bitmap_index PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/io/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/include
)

# 链接 Roaring 库
# In Bitmap/src/CMakeLists.txt
target_link_libraries(bitmap_index PUBLIC roaring::roaring) # Changed PRIVATE to PUBLIC

# 可以设置 C++ 标准等
target_compile_features(bitmap_index PRIVATE cxx_std_17)