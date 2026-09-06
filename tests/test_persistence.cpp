#include "../src/index_store.h"

#include <cstdio>
#include <fstream>

#include "../src/bm25.h"
#include "../src/inverted_index.h"
#include "test_framework.h"

static const char* kTmp = "/tmp/mg_test_snapshot.bin";

static InvertedIndex build_sample() {
    InvertedIndex idx;
    idx.add_document(0, "Alpha", "search engine ranking bm25");
    idx.add_document(1, "Beta", "ranking ranking pagerank graph");
    idx.add_document(2, "Gamma", "inverted index maps terms to documents");
    return idx;
}

TEST(persistence_round_trip_preserves_index) {
    InvertedIndex original = build_sample();
    std::vector<double> pr = {0.5, 0.3, 0.2};

    CHECK(save_snapshot(kTmp, original, pr));

    InvertedIndex loaded;
    std::vector<double> pr_loaded;
    CHECK(load_snapshot(kTmp, loaded, pr_loaded));

    // Metadate globale.
    CHECK_EQ(loaded.num_docs(), original.num_docs());
    CHECK_NEAR(loaded.avg_doc_length(), original.avg_doc_length(), 1e-9);

    // Document frequency pentru cativa termeni.
    CHECK_EQ(loaded.df("ranking"), original.df("ranking"));
    CHECK_EQ(loaded.df("pagerank"), original.df("pagerank"));

    // Postings identice (doc_id + pozitii) pentru un termen repetat.
    const auto* a = original.postings("ranking");
    const auto* b = loaded.postings("ranking");
    CHECK(a != nullptr && b != nullptr);
    CHECK_EQ(a->size(), b->size());
    for (size_t i = 0; i < a->size(); ++i) {
        CHECK_EQ((*a)[i].doc_id, (*b)[i].doc_id);
        CHECK((*a)[i].positions == (*b)[i].positions);
    }

    // Titlurile documentelor.
    CHECK_EQ(loaded.doc(0).title, original.doc(0).title);
    CHECK_EQ(loaded.doc(2).title, original.doc(2).title);

    // Vectorul PageRank.
    CHECK_EQ(pr_loaded.size(), pr.size());
    for (size_t i = 0; i < pr.size(); ++i)
        CHECK_NEAR(pr_loaded[i], pr[i], 1e-12);

    std::remove(kTmp);
}

TEST(persistence_search_identical_after_reload) {
    InvertedIndex original = build_sample();
    std::vector<double> pr = {0.5, 0.3, 0.2};
    save_snapshot(kTmp, original, pr);

    InvertedIndex loaded;
    std::vector<double> pr_loaded;
    load_snapshot(kTmp, loaded, pr_loaded);

    BM25Ranker r1(original), r2(loaded);
    auto res1 = r1.search("ranking documents", 5);
    auto res2 = r2.search("ranking documents", 5);
    CHECK_EQ(res1.size(), res2.size());
    for (size_t i = 0; i < res1.size(); ++i) {
        CHECK_EQ(res1[i].first, res2[i].first);         // acelasi doc
        CHECK_NEAR(res1[i].second, res2[i].second, 1e-9); // acelasi scor
    }
    std::remove(kTmp);
}

TEST(persistence_missing_file_returns_false) {
    InvertedIndex idx;
    std::vector<double> pr;
    CHECK(!load_snapshot("/tmp/does_not_exist_mg.bin", idx, pr));
}

TEST(persistence_bad_magic_returns_false) {
    // Scriem un fisier cu continut invalid.
    {
        std::ofstream out("/tmp/mg_bad.bin", std::ios::binary);
        out << "not a valid index file";
    }
    InvertedIndex idx;
    std::vector<double> pr;
    CHECK(!load_snapshot("/tmp/mg_bad.bin", idx, pr));
    std::remove("/tmp/mg_bad.bin");
}
