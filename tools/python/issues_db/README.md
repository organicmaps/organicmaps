# Issues database

Local JSONL mirror of issues from `organicmaps/organicmaps`. Used for triage —
detecting duplicates, finding already-closed reports, or browsing historical
context without hitting the GitHub API every time.

## Layout

- `fetch_issues.py` — fetch/refresh the database from the GitHub REST API
- `search_issues.py` — token-overlap search over titles and bodies
- `db.py` — shared cache location, migration, and update lock
- `issues.jsonl` — one issue per line (stored in the shared cache directory)
- `fetch_state.json` — cursor for incremental updates (stored beside the database)

## Shared cache

Both tools store data in `$(git rev-parse --path-format=absolute --git-common-dir)/issues_db/`.
All worktrees of a clone use the same database and incremental cursor, regardless
of the current working directory. Separate clones keep separate caches. The cache
is outside the tracked working tree and is not committed.

On first use, an existing `issues.jsonl` beside the invoking script is copied into
the shared cache together with its `fetch_state.json`, if present. For a symlinked
database, the cursor beside the symlink target is used. Existing files are left
in place, and an existing shared database is never overwritten by migration. Run
either tool from the worktree with the most up-to-date cache first.

Set `OM_ISSUES_DB_DIR` to override the cache directory for both tools. When Git is
unavailable or the scripts are outside a checkout, they use the script directory.
Only one fetcher can update a cache at a time; another fetcher exits with an error.
Searches can read the existing database while an update runs. The update lock is
released automatically when the fetcher exits, including after a crash.

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

# Run the worktree-sharing and migration tests (no network requests).
python3 -B -m unittest discover -s tools/python/issues_db -p 'test_*.py'
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
