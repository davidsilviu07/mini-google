#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// N vectori de dimensiune `dim`, stocati contiguu (row-major).
// Vectorul documentului i incepe la data[i*dim].
struct EmbeddingStore {
    uint32_t dim = 0;
    std::vector<float> data;

    int count() const { return dim ? (int)(data.size() / dim) : 0; }
    const float* vec(int i) const { return data.data() + (size_t)i * dim; }
};

// Incarca embeddings-urile documentelor (fisier scris de `embed.py docs`).
// Ordinea vectorilor = ordinea doc_id-urilor din corpus.
bool load_doc_embeddings(const std::string& path, EmbeddingStore& out);

// Incarca embedding-ul unui query plus textul lui (`embed.py query`).
bool load_query_embedding(const std::string& path, std::vector<float>& vec,
                          std::string& text);

// Cosine similarity intre doi vectori de dimensiune `dim`:
//   cos = (a . b) / (|a| * |b|)
// Rezultat in [-1, 1]; 1 = aceeasi directie (sens identic), 0 = ortogonal.
double cosine(const float* a, const float* b, uint32_t dim);

// Cele mai similare `top_k` documente fata de query (dupa cosine),
// sortate descrescator.
std::vector<std::pair<int, double>> vector_search(const EmbeddingStore& store,
                                                  const std::vector<float>& query,
                                                  int top_k);
