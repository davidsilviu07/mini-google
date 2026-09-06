#pragma once
#include <string>
#include <utility>
#include <vector>

#include "inverted_index.h"

// Ranker BM25 -- standardul de-facto pentru relevanta lexicala.
//
// score(D, Q) = sum_{q in Q} IDF(q) * tf(q,D)*(k1+1)
//                                    / ( tf(q,D) + k1*(1 - b + b*|D|/avgdl) )
//
//   IDF(q) = ln( (N - df(q) + 0.5) / (df(q) + 0.5) + 1 )
//
// k1 controleaza saturarea term frequency, b controleaza normalizarea
// dupa lungimea documentului. Valorile 1.2 / 0.75 sunt cele clasice.
class BM25Ranker {
public:
    explicit BM25Ranker(const InvertedIndex& index, double k1 = 1.2,
                        double b = 0.75)
        : idx_(index), k1_(k1), b_(b) {}

    // Intoarce top-k (doc_id, score) sortate descrescator dupa scor.
    std::vector<std::pair<int, double>> search(const std::string& query,
                                               int top_k = 10) const;

private:
    const InvertedIndex& idx_;
    double k1_;
    double b_;
};
