#include "../src/shard.h"

#include "test_framework.h"

TEST(merge_by_score_orders_across_shards) {
    // Rezultate de la 3 shard-uri; combinarea trebuie sa dea ordinea globala.
    std::vector<std::vector<std::pair<std::string, double>>> shards = {
        {{"A", 3.0}, {"B", 1.0}},
        {{"C", 2.5}, {"D", 0.5}},
        {{"E", 4.0}},
    };
    auto m = merge_by_score(shards, 3);
    CHECK_EQ((int)m.size(), 3);
    CHECK_EQ(m[0].first, std::string("E"));  // 4.0
    CHECK_EQ(m[1].first, std::string("A"));  // 3.0
    CHECK_EQ(m[2].first, std::string("C"));  // 2.5
}

TEST(merge_by_score_truncates_to_k) {
    std::vector<std::vector<std::pair<std::string, double>>> shards = {
        {{"A", 3.0}, {"B", 2.0}, {"C", 1.0}},
    };
    CHECK_EQ((int)merge_by_score(shards, 2).size(), 2);
}

TEST(merge_by_score_handles_empty) {
    CHECK(merge_by_score({}, 5).empty());
    std::vector<std::vector<std::pair<std::string, double>>> empties = {{}, {}};
    CHECK(merge_by_score(empties, 5).empty());
}
