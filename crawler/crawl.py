#!/usr/bin/env python3
"""
Wikipedia crawler for mini-google.

Breadth-first crawl over the English Wikipedia using the official MediaWiki
API (not HTML scraping). Starting from one or more seed articles, it follows
internal links until it has collected the requested number of pages, then
writes each page to data/corpus/ in the format the C++ engine reads:

    line 1 : the article title
    body   : the plain-text extract of the article
    tail   : the internal links, as [[Title]] tokens

Only links whose target was also crawled are kept, so the link graph is
self-contained and PageRank is meaningful.

Usage:
    python3 crawler/crawl.py --pages 2000 --seed "Search engine"
    python3 crawler/crawl.py --pages 5000 --seed "Computer science,Algorithm"
"""

import argparse
import os
import re
import sys
import time
from collections import deque

import requests

API = "https://en.wikipedia.org/w/api.php"
HEADERS = {
   
    "User-Agent": "mini-google-crawler/0.1 (https://github.com/davidsilviu07; davidsilviu900@example.com)"
}


def fetch_page(session, title, max_retries=5):
    """Fetch one article's plain text and its namespace-0 (article) links.

    Retries on HTTP 429 (Too Many Requests) with exponential backoff, honoring
    the server's Retry-After header when present. This is what keeps a large
    crawl from getting stuck once the API starts rate limiting us.
    """
    params = {
        "action": "query",
        "format": "json",
        "prop": "extracts|links",
        "explaintext": 1,          # plain text, not HTML
        "exsectionformat": "plain",
        "plnamespace": 0,          # only links to real articles
        "pllimit": "max",          # up to 500 links in one request
        "redirects": 1,            # follow redirects to the canonical title
        "titles": title,
    }

    backoff = 1.0
    for attempt in range(max_retries):
        r = session.get(API, params=params, headers=HEADERS, timeout=30)

        if r.status_code == 429:
            # Server is telling us to slow down. Wait, then retry the SAME page.
            retry_after = r.headers.get("Retry-After")
            wait = float(retry_after) if retry_after else backoff
            print(f"  429 rate limited on '{title}', astept {wait:.0f}s "
                  f"(incercarea {attempt + 1}/{max_retries})", file=sys.stderr)
            time.sleep(wait)
            backoff *= 2               # exponential backoff: 1, 2, 4, 8...
            continue

        r.raise_for_status()
        pages = r.json().get("query", {}).get("pages", {})
        break
    else:
        # Am epuizat incercarile fara succes.
        raise RuntimeError(f"still rate limited after {max_retries} retries")
    for page in pages.values():
        if "missing" in page:      # article does not exist
            return None
        text = page.get("extract", "") or ""
        links = [l["title"] for l in page.get("links", [])]
        return {"title": page["title"], "text": text, "links": links}
    return None


def crawl(seeds, max_pages, delay):
    """BFS from the seeds until max_pages articles are collected."""
    session = requests.Session()
    queue = deque(seeds)
    seen = set(seeds)              # titles ever queued (avoids re-queuing)
    pages = {}                     # title -> {title, text, links}

    while queue and len(pages) < max_pages:
        title = queue.popleft()
        try:
            page = fetch_page(session, title)
        except Exception as e:     # network hiccups should not kill the crawl
            print(f"  skip '{title}': {e}", file=sys.stderr)
            continue

        # Skip empty pages and stubs with almost no text.
        if not page or len(page["text"].strip()) < 200:
            continue

        pages[page["title"]] = page
        print(f"[{len(pages)}/{max_pages}] {page['title']}  "
              f"({len(page['links'])} links)")

        for link in page["links"]:
            if link not in seen:
                seen.add(link)
                queue.append(link)

        time.sleep(delay)          # be polite to the API

    return pages


def sanitize(text):
    """Collapse runs of blank lines so the body stays compact."""
    return re.sub(r"\n{3,}", "\n\n", text).strip()


def write_corpus(pages, out_dir):
    """Write every page as a .txt file in the engine's corpus format."""
    os.makedirs(out_dir, exist_ok=True)
    crawled_titles = set(pages.keys())

    for i, (title, page) in enumerate(sorted(pages.items())):
        # Keep only links pointing at pages we actually crawled (drop self).
        internal, seen = [], set()
        for link in page["links"]:
            if link in crawled_titles and link != title and link not in seen:
                seen.add(link)
                internal.append(link)

        link_line = " ".join(f"[[{l}]]" for l in internal)
        path = os.path.join(out_dir, f"{i:06d}.txt")
        with open(path, "w", encoding="utf-8") as f:
            f.write(title + "\n")
            f.write(sanitize(page["text"]) + "\n")
            if link_line:
                f.write("\n" + link_line + "\n")

    return len(pages)


def main():
    ap = argparse.ArgumentParser(description="Wikipedia crawler for mini-google")
    ap.add_argument("--pages", type=int, default=1000,
                    help="how many articles to collect (default 1000)")
    ap.add_argument("--seed", type=str, default="Search engine",
                    help="comma-separated seed article titles")
    ap.add_argument("--out", type=str, default="data/corpus",
                    help="output directory (default data/corpus)")
    ap.add_argument("--delay", type=float, default=0.5,
                    help="seconds to wait between requests (default 0.2)")
    args = ap.parse_args()

    seeds = [s.strip() for s in args.seed.split(",") if s.strip()]
    print(f"Seeds: {seeds}")
    print(f"Target: {args.pages} pages -> {args.out}\n")

    t0 = time.time()
    pages = crawl(seeds, args.pages, args.delay)
    n = write_corpus(pages, args.out)
    dt = time.time() - t0

    print(f"\nDone: {n} pages written to {args.out} in {dt:.0f}s")
    if n < args.pages:
        print("Note: ran out of reachable links before hitting the target. "
              "Add more --seed articles for a bigger crawl.")


if __name__ == "__main__":
    main()