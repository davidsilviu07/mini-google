#include "bm25.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "tokenizer.h"

std::vector<std::pair<int, double>> BM25Ranker::search(
    const std::string& query, int top_k) const {
    std::vector<std::string> q_terms = tokenize(query);

    const double N = (double)idx_.num_docs();
    const double avgdl = idx_.avg_doc_length();

    // Acumulam scorul pe fiecare document care contine cel putin un termen.
    std::unordered_map<int, double> scores;

    for (const std::string& term : q_terms) {
        const auto* plist = idx_.postings(term);
        if (!plist) continue;  // termen absent din corpus

        const double df = (double)plist->size();
        const double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);

        for (const Posting& p : *plist) {
            const double tf = (double)p.tf();
            const double dl = (double)idx_.doc(p.doc_id).length;
            const double denom = tf + k1_ * (1.0 - b_ + b_ * dl / avgdl);
            scores[p.doc_id] += idf * (tf * (k1_ + 1.0)) / denom;
        }
    }

    std::vector<std::pair<int, double>> results(scores.begin(), scores.end());

    // Nu avem nevoie de sortare completa -- doar top_k, deci partial_sort.
    const int k = std::min<int>(top_k, (int)results.size());
    std::partial_sort(
        results.begin(), results.begin() + k, results.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    results.resize(k);
    return results;
}
