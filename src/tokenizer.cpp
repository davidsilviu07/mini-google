#include "tokenizer.h"
#include <cctype>
#include <unordered_set>

namespace {
// Set mic de stopwords engleze. Le eliminam pentru ca apar peste tot si
// nu ajuta la departajarea documentelor (ar strica scorul BM25).
const std::unordered_set<std::string> kStopwords = {
    "a", "an", "the", "and", "or", "but", "of", "to", "in", "on", "at",
    "for", "with", "is", "are", "was", "were", "be", "been", "being",
    "as", "by", "that", "this", "it", "its", "from", "has", "have", "had"};
}  // namespace

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cur;
    cur.reserve(32);

    auto flush = [&]() {
        if (!cur.empty()) {
            if (kStopwords.find(cur) == kStopwords.end()) {
                tokens.push_back(cur);
            }
            cur.clear();
        }
    };

    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            cur.push_back(static_cast<char>(std::tolower(ch)));
        } else {
            flush();  // orice non-alfanumeric e separator
        }
    }
    flush();
    return tokens;
}
