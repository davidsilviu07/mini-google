#include "shard.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>

#include "bm25.h"
#include "index_store.h"
#include "inverted_index.h"

namespace fs = std::filesystem;

namespace {
struct Raw {
    std::string title;
    std::string body;
};

// Citeste corpusul in ordinea doc_id-urilor (fisiere sortate dupa nume).
std::vector<Raw> read_corpus_local(const std::string& dir) {
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.path().extension() == ".txt") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());

    std::vector<Raw> docs;
    for (const auto& f : files) {
        std::ifstream in(f);
        if (!in) continue;
        std::string title;
        std::getline(in, title);
        std::stringstream body;
        body << in.rdbuf();
        docs.push_back({title, body.str()});
    }
    return docs;
}
}  // namespace

int build_shards(const std::string& corpus_dir, int num_shards,
                 const std::string& prefix) {
    if (num_shards < 1) return 0;
    std::vector<Raw> docs = read_corpus_local(corpus_dir);

    // Fiecare shard e un index independent, cu doc_id-uri locale 0..M-1.
    std::vector<InvertedIndex> shards(num_shards);
    std::vector<int> next_local_id(num_shards, 0);
    for (size_t i = 0; i < docs.size(); ++i) {
        int s = (int)(i % num_shards);
        shards[s].add_document(next_local_id[s]++, docs[i].title, docs[i].body);
    }

    for (int s = 0; s < num_shards; ++s) {
        std::vector<double> no_pagerank;  // calea sharded foloseste doar BM25
        std::string path = prefix + ".shard" + std::to_string(s) + ".bin";
        save_snapshot(path, shards[s], no_pagerank);
    }
    return num_shards;
}

std::vector<std::pair<std::string, double>> query_shard(
    const std::string& snapshot, const std::string& query, int k) {
    InvertedIndex index;
    std::vector<double> pagerank;
    if (!load_snapshot(snapshot, index, pagerank)) return {};

    BM25Ranker ranker(index);
    std::vector<std::pair<std::string, double>> out;
    for (const auto& [id, score] : ranker.search(query, k)) {
        out.push_back({index.doc(id).title, score});
    }
    return out;
}

std::vector<std::pair<std::string, double>> merge_by_score(
    const std::vector<std::vector<std::pair<std::string, double>>>& shard_results,
    int k) {
    std::vector<std::pair<std::string, double>> all;
    for (const auto& r : shard_results) {
        for (const auto& p : r) all.push_back(p);
    }
    int n = std::min<int>(k, (int)all.size());
    std::partial_sort(
        all.begin(), all.begin() + n, all.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    all.resize(n);
    return all;
}

std::vector<std::pair<std::string, double>> distributed_search(
    const std::vector<std::string>& shard_snapshots, const std::string& query,
    int k) {
    // SCATTER: lansam o interogare pe fiecare shard, fiecare pe firul ei.
    std::vector<std::future<std::vector<std::pair<std::string, double>>>> futures;
    for (const auto& snapshot : shard_snapshots) {
        futures.push_back(std::async(std::launch::async, query_shard, snapshot,
                                     query, k));
    }

    // GATHER: strangem rezultatele si le combinam intr-un top-k global.
    std::vector<std::vector<std::pair<std::string, double>>> results;
    for (auto& f : futures) results.push_back(f.get());
    return merge_by_score(results, k);
}
