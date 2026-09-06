#include "../src/vector_search.h"

#include <cstdio>
#include <cstring>
#include <fstream>

#include "../src/binary_io.h"
#include "test_framework.h"

TEST(cosine_identical_is_one) {
    float a[3] = {1, 2, 3}, b[3] = {1, 2, 3};
    CHECK_NEAR(cosine(a, b, 3), 1.0, 1e-9);
}

TEST(cosine_orthogonal_is_zero) {
    float a[2] = {1, 0}, b[2] = {0, 1};
    CHECK_NEAR(cosine(a, b, 2), 0.0, 1e-9);
}

TEST(cosine_opposite_is_minus_one) {
    float a[2] = {1, 1}, b[2] = {-1, -1};
    CHECK_NEAR(cosine(a, b, 2), -1.0, 1e-9);
}

TEST(cosine_ignores_magnitude) {
    // Aceeasi directie, magnitudini diferite -> tot 1.
    float a[2] = {2, 0}, b[2] = {5, 0};
    CHECK_NEAR(cosine(a, b, 2), 1.0, 1e-9);
}

TEST(vector_search_returns_nearest_first) {
    EmbeddingStore s;
    s.dim = 2;
    s.data = {1, 0,   // doc0
              0, 1,   // doc1
              1, 1};  // doc2
    std::vector<float> q = {1, 0};
    auto res = vector_search(s, q, 3);
    CHECK_EQ(res[0].first, 0);            // identic cu query
    CHECK_EQ((int)res.size(), 3);
    CHECK(res[0].second >= res[1].second);  // ordine descrescatoare
}

TEST(embeddings_load_round_trip) {
    const char* path = "/tmp/mg_doc_emb.bin";
    {
        std::ofstream out(path, std::ios::binary);
        out.write("MGEM", 4);
        write_pod<uint32_t>(out, 1);   // version
        write_pod<uint32_t>(out, 2);   // n
        write_pod<uint32_t>(out, 3);   // dim
        float vals[6] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
        out.write(reinterpret_cast<char*>(vals), sizeof(vals));
    }
    EmbeddingStore s;
    CHECK(load_doc_embeddings(path, s));
    CHECK_EQ((int)s.dim, 3);
    CHECK_EQ(s.count(), 2);
    CHECK_NEAR(s.vec(1)[2], 0.6, 1e-6);
    std::remove(path);
}

TEST(query_load_round_trip) {
    const char* path = "/tmp/mg_query.bin";
    {
        std::ofstream out(path, std::ios::binary);
        out.write("MGQE", 4);
        write_pod<uint32_t>(out, 1);   // version
        write_pod<uint32_t>(out, 2);   // dim
        float v[2] = {0.7f, 0.8f};
        out.write(reinterpret_cast<char*>(v), sizeof(v));
        write_string(out, "hello query");
    }
    std::vector<float> vec;
    std::string text;
    CHECK(load_query_embedding(path, vec, text));
    CHECK_EQ((int)vec.size(), 2);
    CHECK_NEAR(vec[0], 0.7, 1e-6);
    CHECK_EQ(text, std::string("hello query"));
    std::remove(path);
}
