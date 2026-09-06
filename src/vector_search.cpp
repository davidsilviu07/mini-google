#include "vector_search.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

#include "binary_io.h"

namespace {
constexpr char kDocMagic[4] = {'M', 'G', 'E', 'M'};
constexpr char kQueryMagic[4] = {'M', 'G', 'Q', 'E'};
constexpr uint32_t kVersion = 1;

bool check_magic(std::istream& in, const char expected[4]) {
    char magic[4];
    in.read(magic, 4);
    return in && std::memcmp(magic, expected, 4) == 0;
}
}  // namespace

bool load_doc_embeddings(const std::string& path, EmbeddingStore& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    if (!check_magic(in, kDocMagic)) return false;
    if (read_pod<uint32_t>(in) != kVersion) return false;

    uint32_t n = read_pod<uint32_t>(in);
    uint32_t dim = read_pod<uint32_t>(in);
    out.dim = dim;
    out.data.resize((size_t)n * dim);
    in.read(reinterpret_cast<char*>(out.data.data()),
            (std::streamsize)out.data.size() * sizeof(float));
    return static_cast<bool>(in);
}

bool load_query_embedding(const std::string& path, std::vector<float>& vec,
                          std::string& text) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    if (!check_magic(in, kQueryMagic)) return false;
    if (read_pod<uint32_t>(in) != kVersion) return false;

    uint32_t dim = read_pod<uint32_t>(in);
    vec.resize(dim);
    in.read(reinterpret_cast<char*>(vec.data()),
            (std::streamsize)dim * sizeof(float));
    text = read_string(in);
    return static_cast<bool>(in);
}

double cosine(const float* a, const float* b, uint32_t dim) {
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (uint32_t i = 0; i < dim; ++i) {
        dot += (double)a[i] * b[i];
        na += (double)a[i] * a[i];
        nb += (double)b[i] * b[i];
    }
    if (na == 0.0 || nb == 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

std::vector<std::pair<int, double>> vector_search(
    const EmbeddingStore& store, const std::vector<float>& query, int top_k) {
    std::vector<std::pair<int, double>> scored;
    scored.reserve(store.count());
    for (int i = 0; i < store.count(); ++i) {
        scored.push_back({i, cosine(store.vec(i), query.data(), store.dim)});
    }
    int k = std::min<int>(top_k, (int)scored.size());
    std::partial_sort(
        scored.begin(), scored.begin() + k, scored.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    scored.resize(k);
    return scored;
}
