#include "inverted_index.h"

#include <istream>
#include <ostream>

#include "binary_io.h"
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

void InvertedIndex::serialize(std::ostream& out) const {
    // 1. Documentele (id, titlu, lungime).
    write_pod<uint64_t>(out, docs_.size());
    for (const auto& [id, info] : docs_) {
        write_pod<int32_t>(out, info.id);
        write_string(out, info.title);
        write_pod<int32_t>(out, info.length);
    }

    // 2. Statisticile globale.
    write_pod<double>(out, avgdl_);
    write_pod<int64_t>(out, static_cast<int64_t>(total_length_));

    // 3. Indexul inversat: pentru fiecare termen, lista de postings.
    write_pod<uint64_t>(out, index_.size());
    for (const auto& [term, postings] : index_) {
        write_string(out, term);
        write_pod<uint64_t>(out, postings.size());
        for (const Posting& p : postings) {
            write_pod<int32_t>(out, p.doc_id);
            write_pod<uint64_t>(out, p.positions.size());
            for (int pos : p.positions) write_pod<int32_t>(out, pos);
        }
    }
}

void InvertedIndex::deserialize(std::istream& in) {
    index_.clear();
    docs_.clear();

    // 1. Documentele.
    uint64_t ndocs = read_pod<uint64_t>(in);
    for (uint64_t i = 0; i < ndocs; ++i) {
        DocInfo info;
        info.id = read_pod<int32_t>(in);
        info.title = read_string(in);
        info.length = read_pod<int32_t>(in);
        docs_[info.id] = info;
    }

    // 2. Statisticile globale.
    avgdl_ = read_pod<double>(in);
    total_length_ = static_cast<long long>(read_pod<int64_t>(in));

    // 3. Indexul inversat.
    uint64_t nterms = read_pod<uint64_t>(in);
    for (uint64_t t = 0; t < nterms; ++t) {
        std::string term = read_string(in);
        uint64_t np = read_pod<uint64_t>(in);
        std::vector<Posting> postings;
        postings.reserve(np);
        for (uint64_t k = 0; k < np; ++k) {
            Posting p;
            p.doc_id = read_pod<int32_t>(in);
            uint64_t npos = read_pod<uint64_t>(in);
            p.positions.resize(npos);
            for (uint64_t x = 0; x < npos; ++x)
                p.positions[x] = read_pod<int32_t>(in);
            postings.push_back(std::move(p));
        }
        index_[term] = std::move(postings);
    }
}
