#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

// O aparitie a unui termen intr-un document.
struct Posting {
    int doc_id;
    std::vector<int> positions;              // pozitiile in document (pt fraze)
    int tf() const { return (int)positions.size(); }  // term frequency
};

// Metadate per document.
struct DocInfo {
    int id;
    std::string title;
    int length = 0;  // numar de termeni (dupa tokenizare)
};

// Inima motorului: mapeaza termen -> lista de postings.
// Tine si statisticile de care are nevoie BM25 (N, avgdl, lungimi).
class InvertedIndex {
public:
    // Adauga un document. `text` e continutul, `title` e afisat in rezultate.
    void add_document(int doc_id, const std::string& title,
                      const std::string& text);

    // Postings pentru un termen (nullptr daca termenul nu exista).
    const std::vector<Posting>* postings(const std::string& term) const;

    // Document frequency: in cate documente apare termenul.
    int df(const std::string& term) const;

    int num_docs() const { return (int)docs_.size(); }
    double avg_doc_length() const { return avgdl_; }
    const DocInfo& doc(int doc_id) const { return docs_.at(doc_id); }

    // Scrie / citeste toata starea interna intr-un/dintr-un flux binar.
    // deserialize() sterge continutul existent inainte de a citi.
    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);

private:
    std::unordered_map<std::string, std::vector<Posting>> index_;
    std::unordered_map<int, DocInfo> docs_;
    double avgdl_ = 0.0;
    long long total_length_ = 0;
};
