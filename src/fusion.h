#pragma once
#include <utility>
#include <vector>

// Reciprocal Rank Fusion (RRF).
//
// Fiecare lista de intrare e deja sortata (cel mai bun rezultat primul) si
// contine doc_id-uri. Un document primeste, din fiecare lista in care apare,
//     1 / (k + rang)
// unde rang incepe de la 0. Scorurile finale se aduna peste toate listele.
//
// Avantajul cheie: NU are nevoie ca scorurile din liste sa fie comparabile --
// foloseste doar pozitia. Asa poti fuziona BM25 (scoruri ~3) cu cosine
// similarity (scoruri ~0.8) fara sa normalizezi nimic. `k` (tipic 60) tempereaza
// influenta pozitiilor foarte de sus.
//
// Intoarce top_k perechi (doc_id, scor_fuzionat) sortate descrescator.
std::vector<std::pair<int, double>> reciprocal_rank_fusion(
    const std::vector<std::vector<int>>& ranked_lists, int top_k,
    double k = 60.0);
