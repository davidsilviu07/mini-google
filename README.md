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

## Crawling real data

The `data/corpus/` shipped with the repo is a tiny sample. To build a large
corpus, run the Wikipedia crawler (needs Python and `requests`):

```bash
pip install -r crawler/requirements.txt
python3 crawler/crawl.py --pages 2000 --seed "Search engine"
```

It performs a breadth first crawl over the English Wikipedia through the
official MediaWiki API, following internal links from the seed articles, and
writes each page into `data/corpus/` in the same title plus body plus links
format the engine reads. Only links between crawled pages are kept, so the
link graph stays self contained and PageRank is meaningful. Use several seeds
(comma separated) and a larger `--pages` for a bigger database.

## Persisting the index

Building the index from a large corpus takes time (reading every file,
tokenizing, computing PageRank). To avoid redoing that on every run, the engine
can save a binary snapshot of the index plus PageRank scores and reload it
instantly.

```bash
./build/search build data/corpus data/index.bin   # build once, save
./build/search load data/index.bin                 # reload instantly
./build/search data/corpus                          # build in memory (no save)
```

On a 20000 page corpus, building takes a couple of seconds while loading the
snapshot takes a fraction of a second, and the gap grows with scale. The
snapshot format starts with a magic number and a version, so a corrupt or
outdated file is rejected on load.

## Hybrid search

BM25 finds lexical matches (shared words). Semantic search finds matches by
meaning: an embedding model turns each document and the query into a dense
vector, and cosine similarity ranks documents by how close their meaning is,
even with no shared words. Hybrid search fuses both signals.

Embeddings are produced offline in Python (that is where the ML model lives);
the C++ engine loads the vectors and does the cosine search and fusion.

```bash
pip install -r embedder/requirements.txt
python3 embedder/embed.py docs  --corpus data/corpus --out data/doc_emb.bin
python3 embedder/embed.py query --text "how do machines learn" --out data/query.bin
./build/search hybrid data/index.bin data/doc_emb.bin data/query.bin
```

The two ranked lists (BM25 and cosine) live on different score scales, so they
are combined with Reciprocal Rank Fusion, which fuses by rank rather than by
raw score and needs no normalization.

## RAG answers

On top of retrieval, the engine can produce a written answer with citations
instead of just a list of links. This is Retrieval-Augmented Generation: fetch
the most relevant documents, hand them to an LLM as the only allowed sources,
and ask it to answer and cite them. Grounding the model in retrieved sources is
what keeps the answer faithful instead of hallucinated.

The C++ engine exposes a machine-readable `retrieve` mode; a Python script does
the orchestration and the LLM call (default: a local Ollama server, no API key).

```bash
pip install -r rag/requirements.txt
./build/search build data/corpus data/index.bin        # once
# see the exact prompt without calling any model:
python3 rag/answer.py "how does pagerank work" --dry-run
# real answer (needs `ollama serve` and a pulled model):
python3 rag/answer.py "how does pagerank work" --model llama3.2
```

The engineering that matters here is the retrieval underneath and the grounding
in the prompt (answer only from the numbered sources, cite them, refuse if they
do not cover the question), not the model itself.

## Distributed index (sharding)

A real search engine's index does not fit on one machine, so it is split into
shards, each an independent index over a subset of the documents. A query is
sent to every shard in parallel (scatter), each returns its local top-k, and a
coordinator merges them into a global top-k (gather).

```bash
./build/search build-shards data/corpus 4 data/idx   # split into 4 shards
./build/search dsearch 5 "how does pagerank work" \
    data/idx.shard0.bin data/idx.shard1.bin data/idx.shard2.bin data/idx.shard3.bin
```

Each shard is queried on its own thread. The merge is by BM25 score; note that
each shard computes IDF over only its own documents, so scores are shard-local
rather than global. That is the standard approximation, and production systems
either accept it or distribute global term statistics to every shard.

## Roadmap

1. Core IR: tokenizer, inverted index, BM25, CLI. Done.
2. PageRank: link graph plus power iteration, combined with BM25.
3. Real corpus: Python crawler over a Wikipedia subset, on disk index.
4. Hybrid search: embeddings plus vector search fused with BM25.
5. RAG: grounded AI answers with citations plus a web UI.
6. Distributed index: sharding plus a parallel scatter gather coordinator. Done.