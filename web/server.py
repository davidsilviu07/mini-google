"""
Web server for mini-google.

Exposes the C++ engine over HTTP: a search endpoint that returns ranked results,
and an optional AI-answer endpoint (RAG) when a Groq key is configured. The
engine binary does the retrieval; this layer just turns queries into HTTP.

Run locally:
    uvicorn web.server:app --reload
Config via environment variables:
    SEARCH_BIN   path to the compiled engine   (default ./search)
    SNAPSHOT     index snapshot to serve        (default data/index.bin)
    CORPUS       corpus dir, for snippets       (default data/corpus)
    GROQ_API_KEY enables AI answers when set
    GROQ_MODEL   model id                        (default openai/gpt-oss-20b)
"""

import os
import re
import subprocess
import time
from pathlib import Path

from fastapi import FastAPI, Query
from fastapi.responses import HTMLResponse

SEARCH_BIN = os.environ.get("SEARCH_BIN", "./search")
SNAPSHOT = os.environ.get("SNAPSHOT", "data/index.bin")
CORPUS = os.environ.get("CORPUS", "data/corpus")
GROQ_API_KEY = os.environ.get("GROQ_API_KEY")
GROQ_MODEL = os.environ.get("GROQ_MODEL", "openai/gpt-oss-20b")
GROQ_URL = "https://api.groq.com/openai/v1/chat/completions"

BASE = Path(__file__).resolve().parent
INDEX_HTML = (BASE / "index.html").read_text(encoding="utf-8")

app = FastAPI(title="mini-google")


def corpus_files():
    try:
        return sorted(f for f in os.listdir(CORPUS) if f.endswith(".txt"))
    except FileNotFoundError:
        return []


def snippet_for(files, doc_id, n=220):
    """First n characters of a document body, links and whitespace cleaned."""
    if not (0 <= doc_id < len(files)):
        return ""
    text = Path(CORPUS, files[doc_id]).read_text(encoding="utf-8", errors="ignore")
    _, _, body = text.partition("\n")
    body = re.sub(r"\[\[.*?\]\]", "", body)
    body = re.sub(r"\s+", " ", body).strip()
    return body[:n]


def snippet_and_words(files, doc_id, n=240):
    """Return (snippet, total_word_count) for a document."""
    if not (0 <= doc_id < len(files)):
        return "", 0
    text = Path(CORPUS, files[doc_id]).read_text(encoding="utf-8", errors="ignore")
    _, _, body = text.partition("\n")
    body = re.sub(r"\[\[.*?\]\]", "", body)
    body = re.sub(r"\s+", " ", body).strip()
    return body[:n], len(body.split())


def do_retrieve(q, k):
    """Run the engine's retrieve mode and parse doc_id<TAB>title lines."""
    out = subprocess.run([SEARCH_BIN, "retrieve", SNAPSHOT, str(k), q],
                         capture_output=True, text=True, timeout=30)
    results = []
    for line in out.stdout.splitlines():
        if "\t" in line:
            doc_id, title = line.split("\t", 1)
            results.append((int(doc_id), title))
    return results


@app.get("/", response_class=HTMLResponse)
def home():
    return INDEX_HTML


@app.get("/api/search")
def api_search(q: str = Query(...), k: int = 8):
    files = corpus_files()
    t0 = time.perf_counter()
    hits = do_retrieve(q, k)
    elapsed_ms = round((time.perf_counter() - t0) * 1000)

    results = []
    for doc_id, title in hits:
        snippet, words = snippet_and_words(files, doc_id)
        results.append({
            "title": title,
            "snippet": snippet,
            "url": "en.wikipedia.org/wiki/" + title.replace(" ", "_"),
            "words": words,
            "read_min": max(1, round(words / 200)),
        })
    return {"query": q, "count": len(results),
            "elapsed_ms": elapsed_ms, "results": results}


@app.get("/api/answer")
def api_answer(q: str = Query(...), k: int = 4):
    if not GROQ_API_KEY:
        return {"answer": None,
                "note": "AI answers are off. Set GROQ_API_KEY on the server to turn them on."}

    files = corpus_files()
    hits = do_retrieve(q, k)
    sources = [{"n": i + 1, "title": t, "text": snippet_for(files, did, 1200)}
               for i, (did, t) in enumerate(hits)]
    context = "\n\n".join(f"[{s['n']}] {s['title']}\n{s['text']}" for s in sources)
    system = ("You are a search assistant. Answer the QUESTION using ONLY the "
              "numbered SOURCES. Cite the ones you use inline, like [1] or [2]. "
              "If they do not contain the answer, say so. No outside knowledge.")
    prompt = f"{system}\n\nSOURCES:\n{context}\n\nQUESTION: {q}\n\nANSWER:"

    import requests
    try:
        r = requests.post(
            GROQ_URL,
            headers={"Authorization": f"Bearer {GROQ_API_KEY}"},
            json={"model": GROQ_MODEL,
                  "messages": [{"role": "user", "content": prompt}],
                  "temperature": 0.2},
            timeout=60)
        r.raise_for_status()
        answer = r.json()["choices"][0]["message"]["content"].strip()
    except Exception as e:
        return {"answer": None, "note": f"AI answer failed: {e}"}

    return {"answer": answer,
            "sources": [{"n": s["n"], "title": s["title"]} for s in sources]}
