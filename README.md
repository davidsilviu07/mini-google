# mini-google

A search engine built from scratch in C++, from classic information retrieval up
to an AI answer layer: a Wikipedia crawler, an inverted index, BM25 and PageRank
ranking, an on-disk index, hybrid semantic search, grounded RAG answers, a
distributed sharded index, and a web interface.

It is a portfolio project that touches data structures, algorithms, information
retrieval, distributed systems, and applied AI. Every core component is covered
by unit tests (48 and counting).

## What it does

- Crawls the English Wikipedia into a local corpus.
- Builds an inverted index with term positions.
- Ranks results with BM25 (lexical) and PageRank (link authority), combined.
- Saves the index to a binary snapshot and reloads it instantly.
- Adds semantic search with embeddings, fused with BM25 (hybrid search).
- Produces written answers with citations over the corpus (RAG).
- Splits the index into shards queried in parallel (scatter-gather).
- Serves all of it behind a small web UI.

## Architecture

```
Wikipedia (MediaWiki API)
      |
      v  crawler (Python)
corpus (.txt files: title + body + [[links]])
      |
      v  tokenizer            normalize + stopword removal
inverted index                term -> postings (doc_id, positions) + doc stats
      |                              |
      v  BM25 ranker                 v  link graph -> PageRank (power iteration)
      \______________  combined  ____/
                      |
   embeddings (Python) + cosine vector search
                      |
      v  Reciprocal Rank Fusion (hybrid)
                      |
      v  RAG: top-k sources -> LLM -> grounded answer with citations
                      |
      v  web UI  /  distributed shards (parallel scatter-gather)
```

## Build and run

The engine is plain C++17. Compile all sources (needs `-pthread` for the
distributed search):

```bash
g++ -std=c++17 -O2 -pthread src/*.cpp -o search
./search build data/corpus data/index.bin
./search load data/index.bin
```

Or with CMake:

```bash
cmake -B build && cmake --build build
./build/search build data/corpus data/index.bin
```

Each file in `data/corpus/` is one document: the first line is the title, the
rest is the body, and `[[Title]]` markers are internal links used by PageRank.

## Tests

```bash
cmake -B build && cmake --build build
cd build && ctest
```

## Crawling real data

The corpus shipped with the repo is a small sample. To build a large corpus,
run the Wikipedia crawler (needs Python and `requests`):

```bash
pip install -r crawler/requirements.txt
python3 crawler/crawl.py --pages 2000 --seed "Computer science,Algorithm"
```

It does a breadth-first crawl over the English Wikipedia through the official
MediaWiki API, following internal links from the seeds, and writes each page
into `data/corpus/` in the format the engine reads. Only links between crawled
pages are kept, so the link graph is self-contained and PageRank is meaningful.
It backs off on rate limits (HTTP 429). Use several seeds and a larger `--pages`
for a bigger database.

## Ranking: BM25 and PageRank

BM25 scores how well a document's text matches the query (term frequency with
saturation, inverse document frequency, length normalization). PageRank scores
how authoritative a page is, from the link graph, using power iteration with a
damping factor and dangling-node handling. The final score multiplies them, so
authority lifts relevant results rather than overriding them.

## Persisting the index

Building the index from a large corpus takes time (reading every file,
tokenizing, computing PageRank). The engine can save a binary snapshot of the
index plus PageRank scores and reload it instantly.

```bash
./search build data/corpus data/index.bin   # build once, save
./search load data/index.bin                 # reload instantly
```

On a 20000 page corpus, building takes a couple of seconds while loading the
snapshot takes a fraction of a second, and the gap grows with scale. The
snapshot starts with a magic number and a version, so a corrupt or outdated
file is rejected on load.

## Hybrid search

BM25 finds lexical matches (shared words). Semantic search finds matches by
meaning: an embedding model turns each document and the query into a dense
vector, and cosine similarity ranks by how close their meaning is, even with no
shared words. Hybrid search fuses both.

Embeddings are produced offline in Python (that is where the ML model lives);
the C++ engine loads the vectors and does the cosine search and fusion.

```bash
pip install -r embedder/requirements.txt
python3 embedder/embed.py docs  --corpus data/corpus --out data/doc_emb.bin
python3 embedder/embed.py query --text "how do machines learn" --out data/query.bin
./search hybrid data/index.bin data/doc_emb.bin data/query.bin
```

The two ranked lists live on different score scales, so they are combined with
Reciprocal Rank Fusion, which fuses by rank rather than raw score and needs no
normalization.

## RAG answers

On top of retrieval, the engine can produce a written answer with citations
instead of a list of links. This is Retrieval-Augmented Generation: fetch the
most relevant documents, hand them to an LLM as the only allowed sources, and
ask it to answer and cite them. Grounding the model in retrieved sources keeps
the answer faithful instead of hallucinated.

The C++ engine exposes a machine-readable `retrieve` mode; a Python script does
the orchestration and the LLM call through an OpenAI-compatible API (default:
Groq, free tier). The LLM provider is configurable, so it is not locked to one
vendor.

```bash
pip install -r rag/requirements.txt
export GROQ_API_KEY=your_key           # from console.groq.com
./search build data/corpus data/index.bin
# see the exact prompt without calling any model:
python3 rag/answer.py "how does pagerank work" --dry-run
# real grounded answer with citations:
python3 rag/answer.py "how does pagerank work"
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
./search build-shards data/corpus 4 data/idx
./search dsearch 5 "how does pagerank work" \
    data/idx.shard0.bin data/idx.shard1.bin data/idx.shard2.bin data/idx.shard3.bin
```

Each shard is queried on its own thread. The merge is by BM25 score; each shard
computes IDF over only its own documents, so scores are shard-local rather than
global. That is the standard approximation, and production systems either accept
it or distribute global term statistics to every shard.

## Web app

A small FastAPI server puts a search box in front of the engine. It serves a
one-page UI and two JSON endpoints: `/api/search` returns ranked results with
snippets, and `/api/answer` returns a grounded RAG answer when a Groq key is set.

```bash
pip install -r web/requirements.txt
g++ -std=c++17 -O2 -pthread src/*.cpp -o search
./search build data/corpus data/index.bin
export GROQ_API_KEY=your_key            # optional, enables AI answers
uvicorn web.server:app --reload         # open http://localhost:8000
```

The whole app is containerized with the included `Dockerfile`, so it runs the
same locally and in the cloud:

```bash
docker build -t mini-google .
docker run -p 8000:8000 -e GROQ_API_KEY=$GROQ_API_KEY mini-google
```

## Layout

```
src/        the C++ engine (index, BM25, PageRank, persistence, vectors, shards)
tests/      unit tests (dependency-free harness)
crawler/    Wikipedia crawler (Python)
embedder/   embedding generator (Python)
rag/        RAG orchestrator (Python)
web/        FastAPI server + web UI
data/       corpus and index snapshots
```
