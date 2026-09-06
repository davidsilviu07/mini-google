#include "../src/pagerank.h"

#include <algorithm>
#include <numeric>

#include "../src/link_graph.h"
#include "test_framework.h"

TEST(pagerank_sums_to_one) {
    LinkGraph g(4);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    g.add_edge(3, 0);
    auto pr = compute_pagerank(g);
    double sum = std::accumulate(pr.begin(), pr.end(), 0.0);
    CHECK_NEAR(sum, 1.0, 1e-6);
}

TEST(pagerank_symmetric_two_nodes_equal) {
    // A <-> B: prin simetrie, scoruri egale.
    LinkGraph g(2);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    auto pr = compute_pagerank(g);
    CHECK_NEAR(pr[0], 0.5, 1e-6);
    CHECK_NEAR(pr[1], 0.5, 1e-6);
}

TEST(pagerank_more_inlinks_higher_score) {
    // 1, 2, 3 pointeaza toate spre 0 -> 0 e cel mai autoritar.
    LinkGraph g(4);
    g.add_edge(1, 0);
    g.add_edge(2, 0);
    g.add_edge(3, 0);
    auto pr = compute_pagerank(g);
    int best = (int)(std::max_element(pr.begin(), pr.end()) - pr.begin());
    CHECK_EQ(best, 0);
}

TEST(pagerank_handles_dangling_node) {
    // Nodul 1 nu are linkuri de iesire (dangling). Suma trebuie sa ramana 1.
    LinkGraph g(2);
    g.add_edge(0, 1);
    auto pr = compute_pagerank(g);
    double sum = std::accumulate(pr.begin(), pr.end(), 0.0);
    CHECK_NEAR(sum, 1.0, 1e-6);
    // 1 primeste linkul lui 0, deci ar trebui sa aiba scor >= 0.
    CHECK(pr[1] > pr[0]);
}

TEST(pagerank_empty_graph) {
    LinkGraph g(0);
    CHECK(compute_pagerank(g).empty());
}
