#include "index_store.h"

#include <cstring>
#include <fstream>

#include "binary_io.h"

namespace {
constexpr char kMagic[4] = {'M', 'G', 'I', 'X'};
constexpr uint32_t kVersion = 1;
}  // namespace

bool save_snapshot(const std::string& path, const InvertedIndex& index,
                   const std::vector<double>& pagerank) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    out.write(kMagic, 4);
    write_pod<uint32_t>(out, kVersion);

    index.serialize(out);

    write_pod<uint64_t>(out, pagerank.size());
    for (double score : pagerank) write_pod<double>(out, score);

    return static_cast<bool>(out);
}

bool load_snapshot(const std::string& path, InvertedIndex& index,
                   std::vector<double>& pagerank) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    char magic[4];
    in.read(magic, 4);
    if (!in || std::memcmp(magic, kMagic, 4) != 0) return false;

    uint32_t version = read_pod<uint32_t>(in);
    if (version != kVersion) return false;

    index.deserialize(in);

    uint64_t n = read_pod<uint64_t>(in);
    pagerank.resize(n);
    for (uint64_t i = 0; i < n; ++i) pagerank[i] = read_pod<double>(in);

    return static_cast<bool>(in);
}
