cmake_minimum_required(VERSION 3.7)
project(Test LANGUAGES CXX)

add_library(compile_flags_interface INTERFACE)
target_compile_features(compile_flags_interface INTERFACE cxx_std_17)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy" "clang-tidy.exe")

if(CLANG_TIDY_EXE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" -p "${CMAKE_BINARY_DIR}" --config-file "${CMAKE_SOURCE_DIR}/.clang-tidy")
else()
    message(WARNING "clang-tidy not found, skipping it")
endif()

add_subdirectory(src)