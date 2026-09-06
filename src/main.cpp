#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "bm25.h"
#include "fusion.h"
#include "index_store.h"
#include "inverted_index.h"
#include "link_graph.h"
#include "pagerank.h"
#include "shard.h"
#include "vector_search.h"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct RawDoc {
    int id;
    std::string title;
    std::string body;
};

// Citeste fisierele .txt: prima linie = titlu, restul = corp (cu linkuri [[..]]).
static std::vector<RawDoc> read_corpus(const std::string& dir) {
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.path().extension() == ".txt") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());

    std::vector<RawDoc> docs;
    int id = 0;
    for (const auto& f : files) {
        std::ifstream in(f);
        if (!in) continue;
        std::string title;
        std::getline(in, title);
        std::stringstream body;
        body << in.rdbuf();
        docs.push_back({id++, title, body.str()});
    }
    return docs;
}

// Construieste index + PageRank dintr-un director de corpus.
static void build_from_corpus(const std::string& dir, InvertedIndex& index,
                              std::vector<double>& pagerank) {
    std::vector<RawDoc> docs = read_corpus(dir);
    const int N = (int)docs.size();

    std::unordered_map<std::string, int> title_to_id;
    for (const auto& d : docs) {
        index.add_document(d.id, d.title, d.body);
        title_to_id[d.title] = d.id;
    }

    LinkGraph graph(N);
    for (const auto& d : docs) {
        for (const std::string& target : extract_links(d.body)) {
            auto it = title_to_id.find(target);
            if (it != title_to_id.end()) graph.add_edge(d.id, it->second);
        }
    }
    pagerank = compute_pagerank(graph);
}

// Ruleaza cateva query-uri demonstrative, cu scor combinat BM25 * PageRank.
static void demo_queries(const InvertedIndex& index,
                         const std::vector<double>& pagerank) {
    double max_pr = pagerank.empty()
                        ? 0.0
                        : *std::max_element(pagerank.begin(), pagerank.end());
    const double alpha = 1.0;
    BM25Ranker ranker(index);

    auto search = [&](const std::string& query) {
        std::cout << "\n> \"" << query << "\"\n";
        auto hits = ranker.search(query, index.num_docs());
        std::vector<std::pair<int, double>> combined;
        for (const auto& [doc_id, bm25] : hits) {
            double pr_norm =
                max_pr > 0 ? pagerank[doc_id] / max_pr : 0.0;
            combined.push_back({doc_id, bm25 * (1.0 + alpha * pr_norm)});
        }
        std::sort(combined.begin(), combined.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        int rank = 1;
        for (const auto& [doc_id, score] : combined) {
            std::cout << "  " << rank++ << ". [" << score << "]  "
                      << index.doc(doc_id).title << "\n";
            if (rank > 5) break;
        }
    };

    search("ranking function for documents");
    search("what does a search engine use");
}

static double seconds_since(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

static void usage() {
    std::cout << "Utilizare:\n"
              << "  search build <corpus_dir> <snapshot>  construieste si salveaza\n"
              << "  search load  <snapshot>               incarca de pe disc\n"
              << "  search hybrid <snapshot> <doc_emb> <query_file>  cautare hibrida\n"
              << "  search retrieve <snapshot> <k> <query>  top-k ca doc_id<TAB>titlu\n"
              << "  search build-shards <corpus_dir> <n> <prefix>  sparge in n shard-uri\n"
              << "  search dsearch <k> <query> <shard...>  cautare distribuita paralela\n"
              << "  search <corpus_dir>                   construieste in memorie\n";
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";

    if (mode == "build") {
        if (argc < 4) { usage(); return 1; }
        std::string corpus = argv[2], snapshot = argv[3];
        InvertedIndex index;
        std::vector<double> pagerank;

        auto t0 = Clock::now();
        build_from_corpus(corpus, index, pagerank);
        std::cout << "Construit din corpus: " << index.num_docs()
                  << " documente in " << seconds_since(t0) << "s\n";

        if (!save_snapshot(snapshot, index, pagerank)) {
            std::cerr << "Eroare la salvarea snapshot-ului.\n";
            return 1;
        }
        std::cout << "Snapshot salvat in " << snapshot << "\n";
        demo_queries(index, pagerank);
        return 0;
    }

    if (mode == "load") {
        if (argc < 3) { usage(); return 1; }
        std::string snapshot = argv[2];
        InvertedIndex index;
        std::vector<double> pagerank;

        auto t0 = Clock::now();
        if (!load_snapshot(snapshot, index, pagerank)) {
            std::cerr << "Eroare la incarcarea snapshot-ului (lipsa/corupt).\n";
            return 1;
        }
        std::cout << "Incarcat de pe disc: " << index.num_docs()
                  << " documente in " << seconds_since(t0) << "s\n";
        demo_queries(index, pagerank);
        return 0;
    }

    if (mode == "hybrid") {
        if (argc < 5) { usage(); return 1; }
        std::string snapshot = argv[2], doc_emb = argv[3], query_file = argv[4];

        InvertedIndex index;
        std::vector<double> pagerank;
        if (!load_snapshot(snapshot, index, pagerank)) {
            std::cerr << "Eroare la incarcarea snapshot-ului.\n";
            return 1;
        }
        EmbeddingStore store;
        if (!load_doc_embeddings(doc_emb, store)) {
            std::cerr << "Eroare la incarcarea embeddings-urilor.\n";
            return 1;
        }
        std::vector<float> qvec;
        std::string qtext;
        if (!load_query_embedding(query_file, qvec, qtext)) {
            std::cerr << "Eroare la incarcarea query-ului.\n";
            return 1;
        }

        // Semnalul lexical (BM25) si cel semantic (cosine), fiecare ca lista
        // ordonata de doc_id-uri.
        BM25Ranker ranker(index);
        std::vector<int> lexical, semantic;
        for (const auto& [id, s] : ranker.search(qtext, index.num_docs()))
            lexical.push_back(id);
        for (const auto& [id, s] : vector_search(store, qvec, store.count()))
            semantic.push_back(id);

        auto fused = reciprocal_rank_fusion({lexical, semantic}, 5);

        std::cout << "Hybrid search pentru: \"" << qtext << "\"\n";
        int rank = 1;
        for (const auto& [id, score] : fused) {
            std::cout << "  " << rank++ << ". [" << score << "]  "
                      << index.doc(id).title << "\n";
        }
        return 0;
    }

    if (mode == "retrieve") {
        // Mod masina-lizibil pentru RAG: scrie top-k ca doc_id<TAB>titlu,
        // cate unul pe linie, fara alt text.
        if (argc < 5) { usage(); return 1; }
        std::string snapshot = argv[2];
        int k = std::atoi(argv[3]);
        std::string query;
        for (int i = 4; i < argc; ++i) {
            if (i > 4) query += " ";
            query += argv[i];
        }
        InvertedIndex index;
        std::vector<double> pagerank;
        if (!load_snapshot(snapshot, index, pagerank)) {
            std::cerr << "Eroare la incarcarea snapshot-ului.\n";
            return 1;
        }
        BM25Ranker ranker(index);
        for (const auto& [id, score] : ranker.search(query, k)) {
            std::cout << id << "\t" << index.doc(id).title << "\n";
        }
        return 0;
    }

    if (mode == "build-shards") {
        if (argc < 5) { usage(); return 1; }
        int n = build_shards(argv[2], std::atoi(argv[3]), argv[4]);
        std::cout << "Am construit " << n << " shard-uri cu prefixul "
                  << argv[4] << "\n";
        return 0;
    }

    if (mode == "dsearch") {
        if (argc < 5) { usage(); return 1; }
        int k = std::atoi(argv[2]);
        std::string query = argv[3];
        std::vector<std::string> shards;
        for (int i = 4; i < argc; ++i) shards.push_back(argv[i]);

        auto t0 = Clock::now();
        auto results = distributed_search(shards, query, k);
        std::cout << "Distributed search pe " << shards.size()
                  << " shard-uri (paralel) in " << seconds_since(t0) << "s:\n";
        int rank = 1;
        for (const auto& [title, score] : results) {
            std::cout << "  " << rank++ << ". [" << score << "]  " << title
                      << "\n";
        }
        return 0;
    }

    // Mod vechi: construieste in memorie si raspunde direct.
    std::string corpus = argc > 1 ? argv[1] : "data/corpus";
    InvertedIndex index;
    std::vector<double> pagerank;
    build_from_corpus(corpus, index, pagerank);
    std::cout << "Index construit in memorie: " << index.num_docs()
              << " documente\n";
    demo_queries(index, pagerank);
    return 0;
}