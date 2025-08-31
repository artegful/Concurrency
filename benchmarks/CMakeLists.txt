include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4
)

option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." OFF)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(benchmarks main.cpp)
target_link_libraries(benchmarks PRIVATE core_library benchmark::benchmark compile_flags_interface)