cmake_minimum_required(VERSION 3.7)
project(Test LANGUAGES CXX)

option(TEST_THREAD_SANITIZE "Add fsanitize=thread option" OFF)
option(TEST_ADDRESS_SANITIZE "Add fsanitize=address option" OFF)
option(TEST_TIDY "Enable clang tidy if possible" OFF)

add_library(compile_flags_interface INTERFACE)
target_compile_features(compile_flags_interface INTERFACE cxx_std_17)

if(TEST_THREAD_SANITIZE)
    target_compile_options(compile_flags_interface INTERFACE -fsanitize=thread)
    target_link_options(compile_flags_interface INTERFACE -fsanitize=thread)
endif()

if(TEST_ADDRESS_SANITIZE)
    target_compile_options(compile_flags_interface INTERFACE -fsanitize=address)
    target_link_options(compile_flags_interface INTERFACE -fsanitize=address)
endif()

if(TEST_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" "clang-tidy.exe")

    if(CLANG_TIDY_EXE)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" -p "${CMAKE_BINARY_DIR}" --config-file "${CMAKE_SOURCE_DIR}/.clang-tidy")
    else()
        message(WARNING "clang-tidy not found, skipping it")
    endif()
endif()

add_subdirectory(src)
add_subdirectory(benchmarks)