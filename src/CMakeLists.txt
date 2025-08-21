file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(core ${SOURCES})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(core PRIVATE compile_flags_interface)
set_target_properties(core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")