cmake_minimum_required(VERSION 3.16)
project(mini_google CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O2 -Wall -Wextra)

set(CORE_SOURCES
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/bm25.cpp
    src/link_graph.cpp
    src/pagerank.cpp
    src/index_store.cpp
    src/vector_search.cpp
    src/fusion.cpp
)

add_executable(search src/main.cpp ${CORE_SOURCES})

# --- teste ---
enable_testing()
add_executable(run_tests
    tests/test_main.cpp
    tests/test_tokenizer.cpp
    tests/test_index.cpp
    tests/test_bm25.cpp
    tests/test_link_graph.cpp
    tests/test_pagerank.cpp
    tests/test_persistence.cpp
    tests/test_vector_search.cpp
    tests/test_fusion.cpp
    ${CORE_SOURCES}
)
add_test(NAME unit_tests COMMAND run_tests)