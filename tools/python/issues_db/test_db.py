"""Integration tests for shared caches; uses temporary Git worktrees and no network."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE_DIR = Path(__file__).resolve().parent
TOOLS_DIR = Path("tools/python/issues_db")
ISSUE = {
    "number": 42,
    "title": "shared crash",
    "body": "A cached issue",
    "html_url": "https://github.com/organicmaps/organicmaps/issues/42",
    "state": "closed",
    "updated_at": "2026-10-21T12:00:00Z",
    "comments": 0,
}
STATE = {"last_updated_at": ISSUE["updated_at"], "total_issues": 1}


class SharedDatabaseTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="issues-db-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.env = os.environ.copy()
        self.env.pop("OM_ISSUES_DB_DIR", None)
        self.repo = self.root / "repo"
        self.repo.mkdir()
        self.git("init")
        scripts = self.repo / TOOLS_DIR
        scripts.mkdir(parents=True)
        for name in ("db.py", "fetch_issues.py", "search_issues.py"):
            shutil.copyfile(SOURCE_DIR / name, scripts / name)
        self.git("add", ".")
        self.git("commit", "-m", "[tools] Create issues database test fixture")
        self.linked = self.root / "linked"
        self.git("worktree", "add", "--detach", str(self.linked))
        self.shared = self.repo / ".git/issues_db"

    def git(self, *args):
        return subprocess.run(
            ["git", "-c", "user.name=Issues DB tests", "-c", "user.email=issues-db@example.invalid",
             "-c", "commit.gpgsign=false", "-c", f"core.hooksPath={self.root / 'no-hooks'}",
             "-C", str(self.repo), *args],
            env=self.env, check=True, capture_output=True, text=True,
        )

    def python(self, tree, code, *, env=None, check=True):
        return subprocess.run(
            [sys.executable, "-B", "-c", "import sys; sys.path.insert(0, sys.argv[1]); " + code,
             str(tree / TOOLS_DIR)],
            cwd=self.root, env=env or self.env, check=check, capture_output=True, text=True, timeout=20,
        )

    def search(self, tree):
        return subprocess.run(
            [sys.executable, "-B", str(tree / TOOLS_DIR / "search_issues.py"), "shared crash"],
            cwd=self.root, env=self.env, check=True, capture_output=True, text=True, timeout=20,
        )

    def fetch(self, tree, *, check=True):
        return self.python(
            tree,
            "import fetch_issues as f; f.get_token = lambda: 'fake'; "
            f"f.request = lambda url, token: ([{ISSUE!r}], {{}}); f.fetch(skip_comments=True)",
            check=check,
        )

    def legacy(self, tree, *, state=STATE):
        directory = tree / TOOLS_DIR
        (directory / "issues.jsonl").write_text(json.dumps(ISSUE) + "\n")
        if state is not None:
            (directory / "fetch_state.json").write_text(json.dumps(state))

    def test_fetch_and_search_share_data_across_worktrees_from_another_directory(self):
        self.fetch(self.repo)
        self.assertIn("#42", self.search(self.linked).stdout)
        self.assertEqual(json.loads((self.shared / "fetch_state.json").read_text()), STATE)
        for tree in (self.repo, self.linked):
            result = self.python(tree, "import db; print(db.DB_DIR)")
            self.assertEqual(Path(result.stdout.strip()), self.shared)
            self.assertFalse((tree / TOOLS_DIR / "issues.jsonl").exists())

    def test_search_migrates_legacy_cache_and_fetch_reuses_its_cursor(self):
        self.legacy(self.linked)
        self.assertIn("#42", self.search(self.linked).stdout)
        self.assertEqual(json.loads((self.shared / "fetch_state.json").read_text()), STATE)
        result = self.python(
            self.repo,
            "import fetch_issues as f; f.get_token = lambda: 'fake'; "
            "f.request = lambda url, token: (print(url) or [], {}); f.fetch(skip_comments=True)",
        )
        self.assertIn("since=" + ISSUE["updated_at"], result.stdout)
        self.assertTrue((self.linked / TOOLS_DIR / "issues.jsonl").is_file())

    def test_legacy_cache_without_cursor_preserves_data_and_does_not_invent_progress(self):
        self.legacy(self.repo, state=None)
        self.assertIn("#42", self.search(self.repo).stdout)
        self.assertFalse((self.shared / "fetch_state.json").exists())
        self.assertEqual(json.loads((self.shared / "issues.jsonl").read_text()), ISSUE)

    def test_existing_shared_cache_is_not_replaced_by_a_legacy_cache(self):
        self.fetch(self.repo)
        legacy_dir = self.linked / TOOLS_DIR
        (legacy_dir / "issues.jsonl").write_text('{"number": 99}\n')
        (legacy_dir / "fetch_state.json").write_text('{"last_updated_at": "2099-01-01T00:00:00Z"}')
        self.assertIn("#42", self.search(self.linked).stdout)
        self.assertEqual(json.loads((self.shared / "fetch_state.json").read_text()), STATE)

    @unittest.skipIf(os.name == "nt", "Creating symlinks may require administrator privileges on Windows")
    def test_symlink_migration_uses_target_cursor_and_updates_keep_legacy_symlink(self):
        self.legacy(self.repo)
        link = self.linked / TOOLS_DIR / "issues.jsonl"
        link.symlink_to(self.repo / TOOLS_DIR / "issues.jsonl")
        (link.parent / "fetch_state.json").write_text('{"last_updated_at": "2099-01-01T00:00:00Z"}')
        self.assertIn("#42", self.search(self.linked).stdout)
        self.assertEqual(json.loads((self.shared / "fetch_state.json").read_text()), STATE)
        self.fetch(self.linked)
        self.assertTrue(link.is_symlink())
        self.assertFalse((self.shared / "issues.jsonl").is_symlink())
        self.assertEqual(json.loads(link.read_text()), ISSUE)

    def test_concurrent_fetch_is_rejected_and_crashed_writer_releases_lock(self):
        self.fetch(self.repo)
        code = (
            "import sys; sys.path.insert(0, sys.argv[1]); import db\n"
            "with db.update_lock():\n"
            "    print('locked', flush=True)\n"
            "    sys.stdin.read()\n"
        )
        with subprocess.Popen(
            [sys.executable, "-B", "-c", code, str(self.repo / TOOLS_DIR)],
            cwd=self.root, env=self.env, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        ) as holder:
            try:
                self.assertEqual(holder.stdout.readline(), "locked\n")
                result = self.fetch(self.linked, check=False)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("being updated by another process", result.stderr)
                self.assertIn("#42", self.search(self.linked).stdout)
            finally:
                holder.kill()
                holder.communicate(timeout=10)
        self.fetch(self.linked)

    def test_directory_override_is_used_by_both_tools(self):
        override = self.root / "custom-cache"
        env = dict(self.env, OM_ISSUES_DB_DIR=str(override))
        for tree in (self.repo, self.linked):
            result = self.python(
                tree,
                "import fetch_issues, search_issues; "
                "print(fetch_issues.DB_PATH); print(search_issues.DB_PATH)",
                env=env,
            )
            self.assertEqual(result.stdout.splitlines(), [str(override / "issues.jsonl")] * 2)

    def test_standalone_scripts_fall_back_to_the_script_directory(self):
        standalone = self.root / "standalone"
        scripts = standalone / TOOLS_DIR
        scripts.mkdir(parents=True)
        for name in ("db.py", "fetch_issues.py", "search_issues.py"):
            shutil.copyfile(SOURCE_DIR / name, scripts / name)
        self.legacy(standalone)
        self.assertIn("#42", self.search(standalone).stdout)
        result = self.python(standalone, "import db; print(db.DB_DIR)")
        self.assertEqual(Path(result.stdout.strip()), scripts)


if __name__ == "__main__":
    unittest.main()
