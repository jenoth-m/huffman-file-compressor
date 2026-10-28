cmake_minimum_required(VERSION 3.20)
project(HuffmanFileCompressor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Both executables use the same compression and file-handling code.
add_library(huffman src/huffman.cpp src/file_io.cpp)
target_include_directories(huffman PUBLIC include)

add_executable(huff src/main.cpp)
target_link_libraries(huff PRIVATE huffman)

enable_testing()
add_executable(codec_tests tests/test_codec.cpp)
target_link_libraries(codec_tests PRIVATE huffman)
add_test(NAME codec_tests COMMAND codec_tests)
