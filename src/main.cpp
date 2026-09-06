#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "bm25.h"
#include "inverted_index.h"
#include "link_graph.h"
#include "pagerank.h"

namespace fs = std::filesystem;

struct RawDoc {
    int id;
    std::string title;
    std::string body;
};

// Citeste toate fisierele .txt: prima linie = titlu, restul = continut.
static std::vector<RawDoc> read_corpus(const std::string& dir) {
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.path().extension() == ".txt") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());  // doc_id-uri stabile

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

int main(int argc, char** argv) {
    const std::string corpus_dir = argc > 1 ? argv[1] : "data/corpus";
    std::vector<RawDoc> docs = read_corpus(corpus_dir);
    const int N = (int)docs.size();

    // 1. Construim indexul si maparea titlu -> doc_id.
    InvertedIndex index;
    std::unordered_map<std::string, int> title_to_id;
    for (const auto& d : docs) {
        index.add_document(d.id, d.title, d.body);
        title_to_id[d.title] = d.id;
    }

    // 2. Construim graful de linkuri din [[Titlu]].
    LinkGraph graph(N);
    for (const auto& d : docs) {
        for (const std::string& target : extract_links(d.body)) {
            auto it = title_to_id.find(target);
            if (it != title_to_id.end()) graph.add_edge(d.id, it->second);
        }
    }

    // 3. Calculam PageRank.
    std::vector<double> pr = compute_pagerank(graph);
    double max_pr = *std::max_element(pr.begin(), pr.end());

    std::cout << "PageRank (autoritate) pe " << N << " documente:\n";
    // Afisam sortat descrescator dupa PR.
    std::vector<int> order(N);
    for (int i = 0; i < N; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return pr[a] > pr[b]; });
    for (int i : order) {
        std::cout << "  " << pr[i] << "  " << index.doc(i).title << "\n";
    }

    // 4. Cautam si combinam: scor_final = BM25 * (1 + alpha * PR_normalizat).
    //    Autoritatea RIDICA rezultatele relevante, nu domina peste ele.
    const double alpha = 1.0;
    BM25Ranker ranker(index);

    auto search = [&](const std::string& query) {
        std::cout << "\n> \"" << query << "\"\n";
        auto hits = ranker.search(query, N);  // luam toate potrivirile
        std::vector<std::pair<int, double>> combined;
        for (const auto& [doc_id, bm25] : hits) {
            double pr_norm = max_pr > 0 ? pr[doc_id] / max_pr : 0.0;
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
    return 0;
}
