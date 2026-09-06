#include "../src/inverted_index.h"
#include "test_framework.h"

// Construieste un index mic, reutilizat de mai multe teste.
static InvertedIndex build_sample() {
    InvertedIndex idx;
    idx.add_document(0, "Doc A", "search engine ranking");            // 3 termeni
    idx.add_document(1, "Doc B", "ranking ranking pagerank");          // 3 termeni
    idx.add_document(2, "Doc C", "search index index index");         // 4 termeni
    return idx;
}

TEST(index_counts_documents) {
    auto idx = build_sample();
    CHECK_EQ(idx.num_docs(), 3);
}

TEST(index_document_frequency) {
    auto idx = build_sample();
    // "search" apare in Doc A si Doc C -> df = 2
    CHECK_EQ(idx.df("search"), 2);
    // "ranking" apare in Doc A si Doc B -> df = 2
    CHECK_EQ(idx.df("ranking"), 2);
    // "pagerank" doar in Doc B -> df = 1
    CHECK_EQ(idx.df("pagerank"), 1);
    // termen inexistent -> df = 0
    CHECK_EQ(idx.df("nonexistent"), 0);
}

TEST(index_term_frequency_within_doc) {
    auto idx = build_sample();
    const auto* p = idx.postings("ranking");
    CHECK(p != nullptr);
    // Cautam postingul pentru Doc B, unde "ranking" apare de 2 ori.
    int tf_docB = -1;
    for (const auto& post : *p) {
        if (post.doc_id == 1) tf_docB = post.tf();
    }
    CHECK_EQ(tf_docB, 2);
}

TEST(index_records_positions) {
    auto idx = build_sample();
    const auto* p = idx.postings("index");  // in Doc C, pozitiile 1,2,3
    CHECK(p != nullptr);
    CHECK_EQ(p->size(), 1u);                 // apare intr-un singur document
    const Posting& post = (*p)[0];
    CHECK_EQ(post.doc_id, 2);
    CHECK(post.positions == std::vector<int>({1, 2, 3}));
}

TEST(index_average_document_length) {
    auto idx = build_sample();
    // lungimi: 3, 3, 4 -> media = 10/3 = 3.333...
    CHECK_NEAR(idx.avg_doc_length(), 10.0 / 3.0, 1e-9);
}

TEST(index_missing_term_returns_null) {
    auto idx = build_sample();
    CHECK(idx.postings("zzz") == nullptr);
}
