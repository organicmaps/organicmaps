"""Shared cache location and single-writer access for issues database tools."""
from __future__ import annotations

import errno
import os
import shutil
import subprocess
from contextlib import contextmanager
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def get_db_dir() -> Path:
    override = os.environ.get("OM_ISSUES_DB_DIR")
    if override:
        return Path(override).expanduser().resolve()
    try:
        common_dir = subprocess.check_output(
            ["git", "-C", str(SCRIPT_DIR), "rev-parse", "--path-format=absolute", "--git-common-dir"],
            stderr=subprocess.PIPE, text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return SCRIPT_DIR
    return Path(common_dir.strip()) / "issues_db"


DB_DIR = get_db_dir()
DB_PATH = DB_DIR / "issues.jsonl"
STATE_PATH = DB_DIR / "fetch_state.json"


def _migrate_legacy_database() -> None:
    """Copy an existing cache once, while holding the shared update lock."""
    legacy_db = (SCRIPT_DIR / "issues.jsonl").resolve()
    if DB_PATH.exists() or not legacy_db.is_file():
        return
    # A symlinked database belongs with its target's cursor, not a local cursor.
    legacy_state = legacy_db.with_name(STATE_PATH.name)
    state_tmp = STATE_PATH.with_suffix(".json.tmp")
    has_state = legacy_state.is_file()
    if has_state:
        shutil.copyfile(legacy_state, state_tmp)
    db_tmp = DB_PATH.with_suffix(".jsonl.tmp")
    shutil.copyfile(legacy_db, db_tmp)
    db_tmp.replace(DB_PATH)
    if has_state:
        state_tmp.replace(STATE_PATH)
    else:
        STATE_PATH.unlink(missing_ok=True)


@contextmanager
def update_lock():
    """Reject concurrent updaters; closing the file also releases a crashed writer's lock."""
    DB_DIR.mkdir(parents=True, exist_ok=True)
    with (DB_DIR / "fetch.lock").open("a+b") as lock:
        try:
            if os.name == "nt":
                import msvcrt

                lock.seek(0)
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl

                fcntl.flock(lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as e:
            if e.errno not in (errno.EACCES, errno.EAGAIN, errno.EDEADLK):
                raise
            raise SystemExit(f"Database is being updated by another process: {DB_DIR}") from e
        _migrate_legacy_database()
        yield


def prepare_database() -> None:
    """Allow searches to reuse a legacy cache without running a fetch first."""
    if not DB_PATH.exists():
        with update_lock():
            pass
