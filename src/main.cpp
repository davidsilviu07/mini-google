#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "bm25.h"
#include "index_store.h"
#include "inverted_index.h"
#include "link_graph.h"
#include "pagerank.h"

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