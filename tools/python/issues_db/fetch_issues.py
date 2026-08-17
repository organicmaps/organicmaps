#!/usr/bin/env python3
"""Fetch all issues from organicmaps/organicmaps and store them in a local JSONL
database. Designed to be reused later to detect duplicate issues or check if a
report has already been fixed/closed.

The script is resumable: it records the highest ``updated_at`` already saved and
fetches only newer/updated issues on subsequent runs (using the ``since``
parameter from the GitHub REST API). Issues are pulled oldest-update first so
the cursor advances monotonically -- an interrupted run resumes without missing
any updates in between.

Fields captured per issue::

    number, title, state, state_reason, labels, created_at, updated_at,
    closed_at, author, comments, body, html_url,
    comments_data, comments_fetched_at

Each entry in ``comments_data`` has: ``id, author, created_at, updated_at, body``.

Pull requests are skipped (the GitHub issues endpoint returns both).

Usage::

    python3 fetch_issues.py                 # incremental update
    python3 fetch_issues.py --full          # discard cache and re-fetch
    python3 fetch_issues.py --skip-comments # skip the comments backfill phase

Authentication: uses ``$GITHUB_TOKEN`` if set, otherwise falls back to the
``gh`` CLI token (``gh auth token``). With a token the quota is 5000 req/h;
unauthenticated is 60 req/h and the script sleeps until reset when exhausted.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

REPO = "organicmaps/organicmaps"
API = f"https://api.github.com/repos/{REPO}/issues"
DB_PATH = Path(__file__).parent / "issues.jsonl"
STATE_PATH = Path(__file__).parent / "fetch_state.json"
PER_PAGE = 100
MAX_RETRIES = 5  # transient (5xx / network) failures to ride out before aborting
FIELDS = (
    "number", "title", "state", "state_reason", "created_at", "updated_at",
    "closed_at", "comments", "body", "html_url",
)
COMMENT_FIELDS = ("id", "created_at", "updated_at", "body")


def get_token() -> str | None:
    """Prefer $GITHUB_TOKEN; otherwise borrow the gh CLI's token if available."""
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        return token
    try:
        out = subprocess.run(
            ["gh", "auth", "token"], capture_output=True, text=True, timeout=10
        )
        if out.returncode == 0:
            return out.stdout.strip() or None
    except (FileNotFoundError, OSError, subprocess.SubprocessError):
        pass
    return None


def _sleep(seconds: int, reason: str) -> None:
    print(f"{reason}; sleeping {seconds}s...", file=sys.stderr, flush=True)
    time.sleep(seconds)


def _reset_wait(headers) -> int:
    """Seconds until the primary rate-limit window resets (with slack)."""
    reset = int(headers.get("X-RateLimit-Reset") or "0")
    return max(reset - int(time.time()), 0) + 5


def request(url: str, token: str | None) -> tuple[list[dict], dict]:
    headers = {
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "organicmaps-issues-db",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    attempt = 0
    while True:
        req = urllib.request.Request(url, headers=headers)
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                # Preserve raw Link header — do not flatten via dict().
                hdr = {
                    "Link": resp.headers.get("Link", ""),
                    "X-RateLimit-Remaining": resp.headers.get("X-RateLimit-Remaining", ""),
                    "X-RateLimit-Reset": resp.headers.get("X-RateLimit-Reset", ""),
                }
                # Proactively wait when the quota is exhausted.
                if int(hdr["X-RateLimit-Remaining"] or "1") == 0:
                    _sleep(_reset_wait(hdr), "Quota exhausted")
                return data, hdr
        except urllib.error.HTTPError as e:
            # Primary rate limit (quota hit 0) or secondary/abuse limit (carries
            # Retry-After). Wait it out and retry; this does not consume the
            # transient-retry budget, since the limit is guaranteed to clear.
            retry_after = e.headers.get("Retry-After")
            if e.code in (403, 429) and (
                    retry_after or e.headers.get("X-RateLimit-Remaining") == "0"):
                wait = (int(retry_after) + 1 if retry_after and retry_after.isdigit()
                        else _reset_wait(e.headers))
                _sleep(wait, f"Rate limited (HTTP {e.code})")
                continue
            # 5xx are transient and worth a bounded retry; other 4xx are not.
            if e.code not in (500, 502, 503, 504) or attempt >= MAX_RETRIES:
                raise
            attempt += 1
            _sleep(2 ** attempt, f"HTTP {e.code}, retry {attempt}/{MAX_RETRIES}")
        except OSError as e:
            # URLError (DNS / connection refused), socket timeouts and connection
            # resets -- any network-level blip. (json.loads raises ValueError,
            # not OSError, so malformed responses are not silently retried.)
            if attempt >= MAX_RETRIES:
                raise
            attempt += 1
            _sleep(2 ** attempt, f"Network error ({e}), retry {attempt}/{MAX_RETRIES}")


def slim(issue: dict) -> dict:
    out = {k: issue.get(k) for k in FIELDS}
    out["labels"] = [lbl["name"] for lbl in issue.get("labels", [])]
    user = issue.get("user") or {}
    out["author"] = user.get("login")
    return out


def slim_comment(comment: dict) -> dict:
    out = {k: comment.get(k) for k in COMMENT_FIELDS}
    user = comment.get("user") or {}
    out["author"] = user.get("login")
    return out


def next_link(headers: dict) -> str | None:
    """Extract the rel=next URL from a GitHub Link header, if present."""
    link = headers.get("Link", "") or headers.get("link", "")
    for part in link.split(","):
        if 'rel="next"' in part:
            return part.split(";")[0].strip().strip("<>")
    return None


def paginate(url: str, token: str | None) -> list[dict]:
    """Follow Link: rel=next pagination, returning the concatenated list."""
    items: list[dict] = []
    while url:
        page, headers = request(url, token)
        items.extend(page)
        url = next_link(headers)
    return items


def fetch_comments(number: int, token: str | None) -> list[dict]:
    url = f"{API}/{number}/comments?per_page={PER_PAGE}"
    return [slim_comment(c) for c in paginate(url, token)]


def needs_comments(issue: dict) -> bool:
    if (issue.get("comments") or 0) == 0:
        return False
    fetched = issue.get("comments_fetched_at")
    if not fetched:
        return True
    # Refetch if issue was updated after comments were last pulled.
    return fetched < (issue.get("updated_at") or "")


def load_state() -> dict:
    if STATE_PATH.exists():
        return json.loads(STATE_PATH.read_text())
    return {}


def save_state(state: dict) -> None:
    STATE_PATH.write_text(json.dumps(state, indent=2))


def rewrite_db(by_number: dict[int, dict]) -> None:
    tmp = DB_PATH.with_suffix(".jsonl.tmp")
    with tmp.open("w") as f:
        for num in sorted(by_number):
            f.write(json.dumps(by_number[num], ensure_ascii=False) + "\n")
    tmp.replace(DB_PATH)


def load_all() -> dict[int, dict]:
    if not DB_PATH.exists():
        return {}
    out = {}
    with DB_PATH.open() as f:
        for line in f:
            try:
                issue = json.loads(line)
                out[issue["number"]] = issue
            except Exception:
                continue
    return out


def fetch(full: bool = False, skip_comments: bool = False) -> None:
    token = get_token()
    if not token:
        print("No token ($GITHUB_TOKEN or `gh auth login`); using the 60 req/h "
              "unauthenticated quota.", file=sys.stderr)
    state = {} if full else load_state()
    by_number = {} if full else load_all()

    # Oldest-update first (direction=asc): updated_at increases monotonically as
    # we paginate, so checkpointing the cursor after every page is safe. If the
    # run is interrupted, the next run resumes from the last saved timestamp and
    # skips nothing. (direction=desc would advance the cursor to the newest
    # timestamp after page 1 and drop everything on the unfetched pages.)
    url = f"{API}?state=all&per_page={PER_PAGE}&sort=updated&direction=asc"
    since = state.get("last_updated_at")
    if since and not full:
        url += f"&since={since}"
        print(f"Incremental fetch since {since}", file=sys.stderr)
    else:
        print("Full fetch", file=sys.stderr)

    fetched = 0
    newest_updated_at = since
    while url:
        page, headers = request(url, token)
        if not page:
            break
        for issue in page:
            if "pull_request" in issue:  # skip PRs
                continue
            num = issue["number"]
            slimmed = slim(issue)
            # Preserve previously-fetched comments; backfill phase refreshes
            # them if the issue was updated after the last comment pull.
            prev = by_number.get(num)
            if prev:
                slimmed["comments_data"] = prev.get("comments_data")
                slimmed["comments_fetched_at"] = prev.get("comments_fetched_at")
            by_number[num] = slimmed
            fetched += 1
            if newest_updated_at is None or issue["updated_at"] > newest_updated_at:
                newest_updated_at = issue["updated_at"]

        remaining = headers.get("X-RateLimit-Remaining", "?")
        print(f"Fetched {fetched} issues so far "
              f"(page size {len(page)}, rate remaining: {remaining})",
              file=sys.stderr, flush=True)

        url = next_link(headers)

        # Persist progress every page. Safe to checkpoint mid-run because pages
        # arrive in ascending updated_at order (see comment above).
        rewrite_db(by_number)
        if newest_updated_at:
            state["last_updated_at"] = newest_updated_at
        state["total_issues"] = len(by_number)
        save_state(state)

    print(f"Issue metadata done: {len(by_number)} issues", file=sys.stderr)

    if skip_comments:
        print(f"Skipping comments backfill at {DB_PATH}", file=sys.stderr)
        return

    # Comments backfill: (re)fetch comments for issues with no cached
    # comments or whose cached comments predate the issue's last update.
    # Sorted oldest-first so interrupted runs resume from the front.
    to_fetch = sorted(n for n, i in by_number.items() if needs_comments(i))
    if not to_fetch:
        print(f"Comments already up-to-date. Database at {DB_PATH}", file=sys.stderr)
        return

    print(f"Backfilling comments for {len(to_fetch)} issues...", file=sys.stderr)
    for i, num in enumerate(to_fetch, 1):
        issue = by_number[num]
        issue["comments_data"] = fetch_comments(num, token)
        issue["comments_fetched_at"] = issue["updated_at"]
        if i % 25 == 0 or i == len(to_fetch):
            rewrite_db(by_number)
            print(f"  comments progress: {i}/{len(to_fetch)}",
                  file=sys.stderr, flush=True)

    print(f"Done. Database at {DB_PATH}", file=sys.stderr)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--full", action="store_true", help="Re-fetch from scratch")
    ap.add_argument("--skip-comments", action="store_true",
                    help="Skip the comments backfill phase")
    args = ap.parse_args()
    fetch(full=args.full, skip_comments=args.skip_comments)


if __name__ == "__main__":
    main()
