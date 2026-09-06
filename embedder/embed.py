#!/usr/bin/env python3
"""
Embedder for mini-google.

Turns text into dense vectors (embeddings) that capture meaning, so the C++
engine can do semantic (vector) search alongside lexical BM25 search.

It has two modes:

  docs   embed every document in the corpus, in the same order the engine
         assigns doc_ids (files sorted by name), and write them to one file.

  query  embed a single query string and write the vector plus the query text,
         so the engine can use the vector for cosine search and the text for
         BM25 in the same run.

Uses the sentence-transformers model all-MiniLM-L6-v2 (384 dimensions),
which runs on CPU. The first run downloads the model (~90 MB).

Usage:
    pip install -r embedder/requirements.txt
    python3 embedder/embed.py docs  --corpus data/corpus --out data/doc_emb.bin
    python3 embedder/embed.py query --text "how do machines learn" --out data/query.bin
"""

import argparse
import os
import struct
import sys

MODEL_NAME = "all-MiniLM-L6-v2"
DOC_MAGIC = b"MGEM"
QUERY_MAGIC = b"MGQE"
VERSION = 1


def load_model():
    # Lazy imports so --help works without the heavy dependencies installed.
    from sentence_transformers import SentenceTransformer
    print(f"Loading model {MODEL_NAME} ...", file=sys.stderr)
    return SentenceTransformer(MODEL_NAME)


def read_corpus_texts(corpus_dir):
    """Read documents in doc_id order (files sorted by name), title + body."""
    files = sorted(f for f in os.listdir(corpus_dir) if f.endswith(".txt"))
    texts = []
    for fn in files:
        with open(os.path.join(corpus_dir, fn), encoding="utf-8") as f:
            content = f.read()
        title, _, body = content.partition("\n")
        texts.append(f"{title}. {body}".strip())
    return texts


def embed_docs(corpus_dir, out_path):
    import numpy as np
    model = load_model()
    texts = read_corpus_texts(corpus_dir)
    if not texts:
        sys.exit(f"No .txt files found in {corpus_dir}")

    emb = model.encode(texts, normalize_embeddings=True,
                       show_progress_bar=True).astype(np.float32)
    n, dim = emb.shape
    with open(out_path, "wb") as f:
        f.write(DOC_MAGIC)
        f.write(struct.pack("<III", VERSION, n, dim))
        f.write(emb.tobytes())  # row-major float32, little-endian
    print(f"Wrote {n} document embeddings of dim {dim} to {out_path}")


def embed_query(text, out_path):
    import numpy as np
    model = load_model()
    vec = model.encode([text], normalize_embeddings=True).astype(np.float32)[0]
    dim = vec.shape[0]
    qbytes = text.encode("utf-8")
    with open(out_path, "wb") as f:
        f.write(QUERY_MAGIC)
        f.write(struct.pack("<II", VERSION, dim))
        f.write(vec.tobytes())
        f.write(struct.pack("<I", len(qbytes)))
        f.write(qbytes)
    print(f"Wrote query embedding (dim {dim}) + text to {out_path}")


def main():
    ap = argparse.ArgumentParser(description="Embedder for mini-google")
    sub = ap.add_subparsers(dest="mode", required=True)

    d = sub.add_parser("docs", help="embed the whole corpus")
    d.add_argument("--corpus", default="data/corpus")
    d.add_argument("--out", default="data/doc_emb.bin")

    q = sub.add_parser("query", help="embed a single query")
    q.add_argument("--text", required=True)
    q.add_argument("--out", default="data/query.bin")

    args = ap.parse_args()
    if args.mode == "docs":
        embed_docs(args.corpus, args.out)
    elif args.mode == "query":
        embed_query(args.text, args.out)


if __name__ == "__main__":
    main()
