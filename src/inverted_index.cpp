#include "inverted_index.h"
#include "tokenizer.h"

void InvertedIndex::add_document(int doc_id, const std::string& title,
                                 const std::string& text) {
    std::vector<std::string> terms = tokenize(text);

    DocInfo info;
    info.id = doc_id;
    info.title = title;
    info.length = (int)terms.size();
    docs_[doc_id] = info;

    // Grupam pozitiile pe termen in cadrul acestui document, ca sa scriem
    // un singur Posting per (termen, doc) in loc de cate unul per aparitie.
    std::unordered_map<std::string, std::vector<int>> local;
    for (int pos = 0; pos < (int)terms.size(); ++pos) {
        local[terms[pos]].push_back(pos);
    }
    for (auto& [term, positions] : local) {
        Posting p;
        p.doc_id = doc_id;
        p.positions = std::move(positions);
        index_[term].push_back(std::move(p));
    }

    // Actualizam avgdl incremental.
    total_length_ += info.length;
    avgdl_ = (double)total_length_ / (double)docs_.size();
}

const std::vector<Posting>* InvertedIndex::postings(
    const std::string& term) const {
    auto it = index_.find(term);
    return it == index_.end() ? nullptr : &it->second;
}

int InvertedIndex::df(const std::string& term) const {
    const auto* p = postings(term);
    return p ? (int)p->size() : 0;
}
