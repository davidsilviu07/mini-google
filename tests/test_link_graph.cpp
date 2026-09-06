#include "../src/link_graph.h"
#include "test_framework.h"

TEST(graph_add_and_query_edges) {
    LinkGraph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    CHECK_EQ(g.out_degree(0), 2);
    CHECK_EQ(g.out_degree(1), 0);
    CHECK(g.out_links(0) == std::vector<int>({1, 2}));
}

TEST(graph_ignores_self_loops) {
    LinkGraph g(2);
    g.add_edge(0, 0);  // trebuie ignorat
    CHECK_EQ(g.out_degree(0), 0);
}

TEST(extract_links_basic) {
    auto l = extract_links("see [[PageRank]] and [[BM25 Ranking]] here");
    CHECK(l == std::vector<std::string>({"PageRank", "BM25 Ranking"}));
}

TEST(extract_links_trims_whitespace) {
    auto l = extract_links("[[  Search Engine  ]]");
    CHECK(l == std::vector<std::string>({"Search Engine"}));
}

TEST(extract_links_none) {
    CHECK(extract_links("no links here at all").empty());
}

TEST(extract_links_unterminated) {
    // "[[" fara "]]" nu trebuie sa arunce eroare sau sa intre in bucla.
    CHECK(extract_links("broken [[link without close").empty());
}
