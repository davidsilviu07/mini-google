#pragma once
#include <vector>

#include "link_graph.h"

// Calculeaza PageRank prin power iteration.
//
//   PR(p) = (1-d)/N  +  d * ( sum_{q->p} PR(q)/L(q)  +  masa_dangling/N )
//
// unde d = damping factor, N = numar de noduri, L(q) = out-degree al lui q.
// Masa "dangling" (nodurile fara linkuri de iesire) e redistribuita uniform,
// altfel s-ar pierde probabilitate si suma n-ar mai fi 1.
//
// Se opreste cand suma diferentelor absolute intre doua iteratii scade sub
// `tol`, sau dupa `max_iter` iteratii.
//
// Intoarce un vector de scoruri (indexat pe doc_id) care insumeaza ~1.0.
std::vector<double> compute_pagerank(const LinkGraph& graph,
                                     double damping = 0.85,
                                     int max_iter = 100,
                                     double tol = 1e-9);
