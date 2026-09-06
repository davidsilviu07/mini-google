cmake_minimum_required(VERSION 3.16)
project(mini_google CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O2 -Wall -Wextra)

add_executable(search
    src/main.cpp
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/bm25.cpp
)

# --- teste ---
enable_testing()
add_executable(run_tests
    tests/test_main.cpp
    tests/test_tokenizer.cpp
    tests/test_index.cpp
    tests/test_bm25.cpp
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/bm25.cpp
)
add_test(NAME unit_tests COMMAND run_tests)
