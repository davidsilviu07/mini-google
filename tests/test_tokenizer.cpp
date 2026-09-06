#include "../src/tokenizer.h"
#include "test_framework.h"

TEST(tokenizer_lowercases) {
    auto t = tokenize("Hello WORLD");
    CHECK_EQ(t.size(), 2u);
    CHECK(t == std::vector<std::string>({"hello", "world"}));
}

TEST(tokenizer_removes_stopwords) {
    // "the" si "and" sunt stopwords -> raman doar "cat" si "dog".
    auto t = tokenize("the cat and the dog");
    CHECK(t == std::vector<std::string>({"cat", "dog"}));
}

TEST(tokenizer_splits_on_punctuation) {
    auto t = tokenize("BM25, PageRank! power-iteration");
    CHECK(t == std::vector<std::string>({"bm25", "pagerank", "power", "iteration"}));
}

TEST(tokenizer_keeps_digits) {
    auto t = tokenize("google 2004 pagerank");
    CHECK(t == std::vector<std::string>({"google", "2004", "pagerank"}));
}

TEST(tokenizer_handles_empty_and_symbols) {
    CHECK(tokenize("").empty());
    CHECK(tokenize("!!! ??? ...").empty());
}

TEST(tokenizer_preserves_position_order) {
    // Ordinea din vector = ordinea din text (importanta pt fraze la V2).
    auto t = tokenize("index maps term document");
    CHECK_EQ(t.size(), 4u);
    CHECK_EQ(t[0], std::string("index"));
    CHECK_EQ(t[3], std::string("document"));
}
