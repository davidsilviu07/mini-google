#include "pagerank.h"

#include <cmath>

std::vector<double> compute_pagerank(const LinkGraph& graph, double damping,
                                     int max_iter, double tol) {
    const int n = graph.num_nodes();
    if (n == 0) return {};

    const double base = (1.0 - damping) / n;  // firimitura de teleport
    std::vector<double> pr(n, 1.0 / n);        // start uniform
    std::vector<double> next(n, 0.0);

    for (int iter = 0; iter < max_iter; ++iter) {
        // 1. Masa din nodurile dangling (fara linkuri de iesire) e
        //    redistribuita uniform catre toate nodurile.
        double dangling = 0.0;
        for (int i = 0; i < n; ++i) {
            if (graph.out_degree(i) == 0) dangling += pr[i];
        }
        const double dangling_share = damping * dangling / n;

        // 2. Fiecare nod porneste de la baza de teleport + partea de dangling.
        for (int i = 0; i < n; ++i) next[i] = base + dangling_share;

        // 3. Fiecare nod isi imparte scorul egal catre nodurile spre care
        //    pointeaza.
        for (int i = 0; i < n; ++i) {
            int deg = graph.out_degree(i);
            if (deg == 0) continue;
            double share = damping * pr[i] / deg;
            for (int j : graph.out_links(i)) next[j] += share;
        }

        // 4. Convergenta: cat s-au miscat scorurile fata de iteratia trecuta?
        double diff = 0.0;
        for (int i = 0; i < n; ++i) diff += std::fabs(next[i] - pr[i]);

        pr.swap(next);
        if (diff < tol) break;
    }
    return pr;
}
