add_executable(bitmap_search_app main.cpp)

# 链接我们自己构建的库
target_link_libraries(bitmap_search_app PRIVATE bitmap_index)