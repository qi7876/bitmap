cmake_minimum_required(VERSION 3.15) # 建议使用较新版本
project(BitmapInvertedIndex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # 建议 C++17 或更高
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 配置 vcpkg (确保 CMAKE_TOOLCHAIN_FILE 指向 vcpkg.cmake)
# 通常在命令行指定: cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
# 或者可以在这里尝试 find_package，但推荐 toolchain 方式

# 添加子目录
add_subdirectory(src)
add_subdirectory(app)

# 启用测试 (如果 Catch2 安装了)
enable_testing()
add_subdirectory(tests)