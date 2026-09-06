#include "fusion.h"

#include <algorithm>
#include <unordered_map>

std::vector<std::pair<int, double>> reciprocal_rank_fusion(
    const std::vector<std::vector<int>>& ranked_lists, int top_k, double k) {
    std::unordered_map<int, double> scores;

    for (const auto& list : ranked_lists) {
        for (size_t rank = 0; rank < list.size(); ++rank) {
            scores[list[rank]] += 1.0 / (k + (double)rank);
        }
    }

    std::vector<std::pair<int, double>> fused(scores.begin(), scores.end());
    int n = std::min<int>(top_k, (int)fused.size());
    std::partial_sort(
        fused.begin(), fused.begin() + n, fused.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    fused.resize(n);
    return fused;
}
