#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "bm25.h"
#include "inverted_index.h"

namespace fs = std::filesystem;

// Fiecare fisier din corpus: prima linie = titlu, restul = continut.
static bool load_document(const fs::path& file, int doc_id,
                          InvertedIndex& index) {
    std::ifstream in(file);
    if (!in) return false;

    std::string title;
    std::getline(in, title);

    std::stringstream body;
    body << in.rdbuf();

    index.add_document(doc_id, title, body.str());
    return true;
}

static int load_corpus(const std::string& dir, InvertedIndex& index) {
    int doc_id = 0;
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".txt") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());  // ordine stabila a doc_id-urilor
    for (const auto& f : files) {
        if (load_document(f, doc_id, index)) ++doc_id;
    }
    return doc_id;
}

static void run_query(const BM25Ranker& ranker, const InvertedIndex& index,
                      const std::string& query) {
    std::cout << "\n> \"" << query << "\"\n";
    auto results = ranker.search(query, 5);
    if (results.empty()) {
        std::cout << "  (niciun rezultat)\n";
        return;
    }
    int rank = 1;
    for (const auto& [doc_id, score] : results) {
        std::cout << "  " << rank++ << ". [" << score << "]  "
                  << index.doc(doc_id).title << "\n";
    }
}

int main(int argc, char** argv) {
    const std::string corpus_dir =
        argc > 1 ? argv[1] : "data/corpus";

    InvertedIndex index;
    int n = load_corpus(corpus_dir, index);
    std::cout << "Index construit: " << n << " documente, avgdl="
              << index.avg_doc_length() << "\n";

    BM25Ranker ranker(index);

    // Cateva query-uri demonstrative.
    run_query(ranker, index, "ranking algorithm for search");
    run_query(ranker, index, "how does google rank pages");
    run_query(ranker, index, "data structure that maps terms to documents");

    // Mod interactiv (daca ruleaza intr-un terminal).
    std::string line;
    std::cout << "\nScrie un query (sau Enter gol pentru iesire):\n";
    while (std::cout << "search> " && std::getline(std::cin, line)) {
        if (line.empty()) break;
        run_query(ranker, index, line);
    }
    return 0;
}
