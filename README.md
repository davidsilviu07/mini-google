# mini-google

A search engine built from scratch in C++: inverted index, BM25 ranking, and
(coming) PageRank, hybrid semantic search, and a distributed sharded index.

The goal is to rebuild the core ideas behind a modern web search engine, from
classic information retrieval up to an AI answer layer.

## Status

Implemented: tokenizer, inverted index, BM25 ranker, and a CLI search
interface, all covered by unit tests.

## Architecture

```
corpus (.txt files)
      |
      v
  tokenizer         normalize + stopword removal
      |
      v
  inverted index    term to postings (doc_id, positions), plus doc stats
      |
      v
  BM25 ranker       IDF, TF saturation, length normalized scoring
      |
      v
  top k results
```

## Build and run

```bash
g++ -std=c++17 -O2 -Wall -Wextra \
    src/main.cpp src/tokenizer.cpp src/inverted_index.cpp src/bm25.cpp \
    -o search
./search data/corpus
```

Or with CMake:

```bash
cmake -B build && cmake --build build
./build/search data/corpus
```

Each file in `data/corpus/` is one document: the first line is the title and
the rest is the body.

## Tests

```bash
cmake -B build && cmake --build build
cd build && ctest
```

## Roadmap

1. Core IR: tokenizer, inverted index, BM25, CLI. Done.
2. PageRank: link graph plus power iteration, combined with BM25.
3. Real corpus: Python crawler over a Wikipedia subset, on disk index.
4. Hybrid search: embeddings plus vector search fused with BM25.
5. RAG: grounded AI answers with citations plus a web UI.
6. Distributed index: sharding, a scatter gather coordinator, fault tolerance.
