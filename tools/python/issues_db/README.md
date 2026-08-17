# Issues database

Local JSONL mirror of issues from `organicmaps/organicmaps`. Used for triage —
detecting duplicates, finding already-closed reports, or browsing historical
context without hitting the GitHub API every time.

## Layout

- `fetch_issues.py` — fetch/refresh the database from the GitHub REST API
- `search_issues.py` — token-overlap search over titles and bodies
- `issues.jsonl` — one issue per line (created on first run; git-ignored)
- `fetch_state.json` — cursor for incremental updates (git-ignored)

## Usage

```bash
# First run populates issues.jsonl (~6000 issues at time of writing), then
# backfills comments for every issue that has any. Auth is taken from
# $GITHUB_TOKEN, or the `gh` CLI token (`gh auth token`) if that is unset —
# either buys 5000 req/h. Unauthenticated is 60 req/h (the script sleeps
# until reset).
python3 tools/python/issues_db/fetch_issues.py

# Subsequent runs only fetch issues updated since the last successful run,
# and re-pull comments for just those issues.
python3 tools/python/issues_db/fetch_issues.py

# Skip the comments phase when you just need metadata quickly.
python3 tools/python/issues_db/fetch_issues.py --skip-comments

# Discard the local cache and re-fetch everything from scratch.
python3 tools/python/issues_db/fetch_issues.py --full

# Search for likely duplicates of a bug you're about to file (matches
# titles, bodies, and comment bodies by default).
python3 tools/python/issues_db/search_issues.py "crash when importing kml"
python3 tools/python/issues_db/search_issues.py --state closed "dark mode battery"
python3 tools/python/issues_db/search_issues.py --label bug "route waypoint"
python3 tools/python/issues_db/search_issues.py --no-comments "route waypoint"
```

## Record schema

```json
{
  "number": 1234,
  "title": "…",
  "state": "open | closed",
  "state_reason": "completed | not_planned | reopened | null",
  "labels": ["bug", "android"],
  "created_at": "2023-01-02T03:04:05Z",
  "updated_at": "2024-05-06T07:08:09Z",
  "closed_at": "2024-05-06T07:08:09Z",
  "author": "username",
  "comments": 3,
  "body": "…",
  "html_url": "https://github.com/organicmaps/organicmaps/issues/1234",
  "comments_fetched_at": "2024-05-06T07:08:09Z",
  "comments_data": [
    {
      "id": 9876543,
      "author": "username",
      "created_at": "2024-05-06T07:08:09Z",
      "updated_at": "2024-05-06T07:08:09Z",
      "body": "…"
    }
  ]
}
```

`comments_fetched_at` mirrors the issue's `updated_at` at the time the
comments were pulled; the fetcher uses it to decide whether a re-pull is
needed on the next run. Pull requests are filtered out; only true issues
are stored.
