#!/usr/bin/env python3
"""
RAG (Retrieval-Augmented Generation) for mini-google.

Takes a question, asks the C++ engine for the most relevant documents, builds a
prompt that hands those documents to an LLM as the ONLY allowed sources, and
prints a grounded answer with citations. This is the pattern behind Google AI
Overviews: the model answers from retrieved sources, not from its own memory,
which is what keeps it from hallucinating.

The LLM call defaults to a local Ollama server (free, no API key). Point --url
and --model at whatever you have running. To use a hosted API instead, replace
the body of call_llm().

Usage:
    # 1) build a snapshot first (see the engine README)
    # 2) see the prompt without calling any model:
    python3 rag/answer.py "how does pagerank work" --dry-run
    # 3) real answer (needs `ollama serve` + a pulled model):
    python3 rag/answer.py "how does pagerank work" --model llama3.2
"""

import argparse
import os
import re
import subprocess
import sys


def corpus_files(corpus_dir):
    """Files in the same order the engine assigns doc_ids (sorted by name)."""
    return sorted(f for f in os.listdir(corpus_dir) if f.endswith(".txt"))


def read_doc(corpus_dir, files, doc_id, max_chars):
    """Return (title, cleaned_body) for a doc_id, trimmed to max_chars."""
    path = os.path.join(corpus_dir, files[doc_id])
    with open(path, encoding="utf-8") as f:
        content = f.read()
    title, _, body = content.partition("\n")
    body = re.sub(r"\[\[.*?\]\]", "", body)      # drop [[link]] markers
    body = re.sub(r"\s+", " ", body).strip()     # collapse whitespace
    return title, body[:max_chars]


def retrieve(engine, snapshot, k, query):
    """Ask the C++ engine for top-k results as (doc_id, title) pairs."""
    out = subprocess.run([engine, "retrieve", snapshot, str(k), query],
                         capture_output=True, text=True, check=True)
    results = []
    for line in out.stdout.splitlines():
        if "\t" in line:
            doc_id, title = line.split("\t", 1)
            results.append((int(doc_id), title))
    return results


def build_prompt(query, sources):
    """Assemble the grounded prompt. `sources` = list of (n, title, text)."""
    context = "\n\n".join(f"[{n}] {title}\n{text}" for n, title, text in sources)
    system = (
        "You are a search assistant. Answer the QUESTION using ONLY the "
        "numbered SOURCES below. Cite the sources you rely on inline, like "
        "[1] or [2]. If the sources do not contain the answer, say you cannot "
        "answer from the given sources. Do not use any outside knowledge."
    )
    return f"{system}\n\nSOURCES:\n{context}\n\nQUESTION: {query}\n\nANSWER:"


def call_llm(prompt, model, url, api_key):
    """Call an OpenAI-compatible chat API (default: Groq). The endpoint, model
    and key are all configurable, so switching providers is a one-line change."""
    import requests
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
    }
    payload = {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": 0.2,   # low = stick to the sources, less improvisation
    }
    r = requests.post(url, headers=headers, json=payload, timeout=120)
    r.raise_for_status()
    return r.json()["choices"][0]["message"]["content"].strip()


def main():
    ap = argparse.ArgumentParser(description="RAG answerer for mini-google")
    ap.add_argument("query", help="the question to answer")
    ap.add_argument("--engine", default="./search")
    ap.add_argument("--snapshot", default="data/index.bin")
    ap.add_argument("--corpus", default="data/corpus")
    ap.add_argument("--k", type=int, default=4, help="how many sources to use")
    ap.add_argument("--model", default="openai/gpt-oss-20b",
                    help="model id (see console.groq.com/docs/models)")
    ap.add_argument("--url",
                    default="https://api.groq.com/openai/v1/chat/completions")
    ap.add_argument("--max-chars", type=int, default=1500,
                    help="trim each source to this many characters")
    ap.add_argument("--dry-run", action="store_true",
                    help="print the prompt and exit, without calling the LLM")
    args = ap.parse_args()

    files = corpus_files(args.corpus)
    hits = retrieve(args.engine, args.snapshot, args.k, args.query)
    if not hits:
        sys.exit("No documents retrieved. Is the snapshot built?")

    sources = []
    for n, (doc_id, title) in enumerate(hits, start=1):
        if 0 <= doc_id < len(files):
            _, body = read_doc(args.corpus, files, doc_id, args.max_chars)
            sources.append((n, title, body))

    prompt = build_prompt(args.query, sources)

    if args.dry_run:
        print(prompt)
        return

    api_key = os.environ.get("GROQ_API_KEY")
    if not api_key:
        sys.exit("Set your API key first:  export GROQ_API_KEY=your_key_here")

    answer = call_llm(prompt, args.model, args.url, api_key)
    print(answer)
    print("\nSources:")
    for n, title, _ in sources:
        print(f"  [{n}] {title}")


if __name__ == "__main__":
    main()