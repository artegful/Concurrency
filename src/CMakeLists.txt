file(GLOB_RECURSE SOURCES "*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(core_library ${SOURCES})
target_include_directories(core_library PUBLIC ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(core_library PRIVATE compile_flags_interface)
set_target_properties(core_library PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")

add_executable(core main.cpp)
target_link_libraries(core PUBLIC core_library PRIVATE compile_flags_interface)
set_target_properties(core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")