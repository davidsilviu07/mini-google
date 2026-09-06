#include "../src/bm25.h"

#include "../src/inverted_index.h"
#include "test_framework.h"

static InvertedIndex build_sample() {
    InvertedIndex idx;
    idx.add_document(0, "Ranking",   "bm25 ranking scores documents by relevance");
    idx.add_document(1, "PageRank",  "pagerank ranks pages using the link graph");
    idx.add_document(2, "Index",     "an inverted index maps terms to documents");
    idx.add_document(3, "Unrelated", "the weather today is sunny and warm");
    return idx;
}

TEST(bm25_ranks_relevant_doc_first) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    auto res = ranker.search("bm25 relevance ranking", 5);
    CHECK(!res.empty());
    CHECK_EQ(res[0].first, 0);  // Doc 0 ("Ranking") trebuie sa fie primul
}

TEST(bm25_matches_only_docs_with_query_terms) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    // "pagerank" apare doar in Doc 1 -> un singur rezultat.
    auto res = ranker.search("pagerank", 5);
    CHECK_EQ(res.size(), 1u);
    CHECK_EQ(res[0].first, 1);
}

TEST(bm25_unrelated_doc_not_returned) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    auto res = ranker.search("inverted index documents", 5);
    // Doc 3 ("Unrelated") nu contine niciun termen din query.
    for (const auto& [doc_id, score] : res) {
        CHECK(doc_id != 3);
    }
}

TEST(bm25_respects_top_k) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    // "documents" apare in Doc 0 si Doc 2, dar cerem doar top 1.
    auto res = ranker.search("documents", 1);
    CHECK_EQ(res.size(), 1u);
}

TEST(bm25_scores_descending) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    auto res = ranker.search("ranking documents pages", 5);
    for (size_t i = 1; i < res.size(); ++i) {
        CHECK(res[i - 1].second >= res[i].second);  // ordine descrescatoare
    }
}

TEST(bm25_empty_query_returns_nothing) {
    auto idx = build_sample();
    BM25Ranker ranker(idx);
    CHECK(ranker.search("", 5).empty());
    CHECK(ranker.search("the and of", 5).empty());  // doar stopwords
}
