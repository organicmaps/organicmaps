#!/usr/bin/env python3
"""Search the local issues database for issues matching query terms.

Useful for triaging: quickly finds likely duplicates or already-closed reports
before opening a new issue.

Usage::

    python3 search_issues.py "route via waypoint crash"
    python3 search_issues.py --state closed --label bug "dark mode"
    python3 search_issues.py --limit 5 "bookmark import KML"

Ranking is a simple token-overlap score over title + body (case-insensitive,
punctuation stripped). It is intentionally dependency-free.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

DB_PATH = Path(__file__).parent / "issues.jsonl"

_TOKEN = re.compile(r"[a-z0-9]+")


def tokenize(text: str) -> list[str]:
    return _TOKEN.findall((text or "").lower())


def score(query_tokens: set[str], issue: dict, search_comments: bool) -> float:
    title_tokens = set(tokenize(issue.get("title", "")))
    body_tokens = set(tokenize((issue.get("body") or "")[:2000]))
    comment_tokens: set[str] = set()
    if search_comments:
        for c in issue.get("comments_data") or []:
            comment_tokens.update(tokenize((c.get("body") or "")[:2000]))
    if not query_tokens:
        return 0.0
    # Score each query token in its highest tier only (title > body > comment)
    # so a token in several fields is not double-counted -- keeps score in [0, 1].
    title_hits = len(query_tokens & title_tokens)
    body_hits = len(query_tokens & (body_tokens - title_tokens))
    comment_hits = len(query_tokens & (comment_tokens - body_tokens - title_tokens))
    # title weighted 3x, body 1x, comment-only 0.5x.
    numer = title_hits * 3 + body_hits + comment_hits * 0.5
    return numer / (len(query_tokens) * 3)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("query", nargs="+", help="Query terms")
    ap.add_argument("--state", choices=("open", "closed", "all"), default="all")
    ap.add_argument("--label", action="append", default=[],
                    help="Require label (repeatable)")
    ap.add_argument("--limit", type=int, default=15)
    ap.add_argument("--min-score", type=float, default=0.15)
    ap.add_argument("--no-comments", action="store_true",
                    help="Skip comment bodies when scoring")
    args = ap.parse_args()

    if not DB_PATH.exists():
        print(f"No database at {DB_PATH}. Run fetch_issues.py first.",
              file=sys.stderr)
        sys.exit(1)

    query_tokens = set(tokenize(" ".join(args.query)))
    required_labels = {lbl.lower() for lbl in args.label}

    results: list[tuple[float, dict]] = []
    with DB_PATH.open() as f:
        for line in f:
            issue = json.loads(line)
            if args.state != "all" and issue.get("state") != args.state:
                continue
            if required_labels:
                labels = {lbl.lower() for lbl in issue.get("labels", [])}
                if not required_labels.issubset(labels):
                    continue
            s = score(query_tokens, issue, not args.no_comments)
            if s >= args.min_score:
                results.append((s, issue))

    results.sort(key=lambda r: (-r[0], -r[1]["number"]))
    for s, issue in results[:args.limit]:
        reason = issue.get("state_reason") or ""
        reason = f" ({reason})" if reason else ""
        print(f"[{s:.2f}] #{issue['number']:<5} {issue['state']:<6}{reason} "
              f"{issue['title']}")
        print(f"        {issue['html_url']}")


if __name__ == "__main__":
    main()
