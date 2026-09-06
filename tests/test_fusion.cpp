#include "../src/fusion.h"

#include "test_framework.h"

TEST(rrf_doc_in_both_lists_wins) {
    // doc 5 e in ambele liste (sus) -> trebuie sa fie primul dupa fuziune.
    std::vector<int> lexical = {5, 1, 2};
    std::vector<int> semantic = {5, 3, 4};
    auto fused = reciprocal_rank_fusion({lexical, semantic}, 10);
    CHECK_EQ(fused[0].first, 5);
}

TEST(rrf_combines_disjoint_lists) {
    std::vector<int> a = {1, 2};
    std::vector<int> b = {3, 4};
    auto fused = reciprocal_rank_fusion({a, b}, 10);
    CHECK_EQ((int)fused.size(), 4);  // toate cele 4 documente apar
}

TEST(rrf_score_matches_formula) {
    // Cu k=60: doc din ambele liste pe pozitia 0 -> 2 * 1/(60+0) = 1/30.
    std::vector<int> a = {7};
    std::vector<int> b = {7};
    auto fused = reciprocal_rank_fusion({a, b}, 1, 60.0);
    CHECK_NEAR(fused[0].second, 2.0 / 60.0, 1e-12);
}

TEST(rrf_respects_top_k) {
    std::vector<int> a = {1, 2, 3, 4, 5};
    auto fused = reciprocal_rank_fusion({a}, 2);
    CHECK_EQ((int)fused.size(), 2);
}

TEST(rrf_empty_input) {
    auto fused = reciprocal_rank_fusion({}, 5);
    CHECK(fused.empty());
}
