"""Exercise merged-branches.sh in disposable repositories, without network access.

Run with: python3 -B tools/test_merged_branches.py
Set BASH to test a specific Bash executable, e.g. BASH=/bin/bash on macOS.
"""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("merged-branches.sh").resolve()
BASH = os.environ.get("BASH", "bash")
GIT = shutil.which("git")


class MergedBranchesTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="merged-branches-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.env = {key: value for key, value in os.environ.items() if not key.startswith("GIT_")}
        self.env.update(GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_NOSYSTEM="1", PARALLEL="2")
        self.git("init", "-q", "-b", "master")
        self.git("config", "user.name", "Test")
        self.git("config", "user.email", "test@example.invalid")
        self.commit("root")

    def git(self, *args):
        return subprocess.run([GIT, *args], cwd=self.root, env=self.env, check=True,
                              text=True, capture_output=True).stdout.strip()

    def commit(self, name, text=None):
        (self.root / name).write_text(text if text is not None else name + "\n")
        self.git("add", name)
        self.git("commit", "-qm", name)
        return self.git("rev-parse", "HEAD")

    def scan(self, *args):
        refs = self.git("show-ref")
        status = self.git("status", "--porcelain")
        result = subprocess.run([BASH, str(SCRIPT), *args], cwd=self.root, env=self.env,
                                text=True, capture_output=True)
        self.assertEqual(self.git("show-ref"), refs)
        self.assertEqual(self.git("status", "--porcelain"), status)
        return result

    def classifications(self, *args):
        result = self.scan(*args)
        self.assertEqual(result.returncode, 0, result.stderr)
        branches = {}
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                branch, separator, method = line.strip().partition("  (")
                branches[branch] = method.removesuffix(")") if separator else "unmerged"
        return branches

    def make_unmerged(self, name="feature"):
        self.git("switch", "-qc", name)
        self.commit("feature")
        self.git("switch", "-q", "master")
        self.commit("main")

    def replace_cherry(self, code):
        # Fault injection affects only script subprocesses inside this fixture.
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        wrapper = bin_dir / "git"
        wrapper.write_text(
            f"#!{sys.executable}\nimport os, sys\n"
            "if sys.argv[1] == 'cherry':\n"
            + "".join("    " + line + "\n" for line in code.splitlines())
            + f"os.execv({GIT!r}, [{GIT!r}, *sys.argv[1:]])\n"
        )
        wrapper.chmod(0o755)
        self.env["PATH"] = str(bin_dir) + os.pathsep + self.env["PATH"]

    def test_base_is_excluded_and_empty_result_succeeds(self):
        for base in ((), ("refs/heads/master",), ("HEAD",)):
            with self.subTest(base=base):
                self.assertEqual(self.classifications(*base), {})

    def test_ancestor(self):
        self.git("branch", "old")
        self.commit("main")
        self.assertEqual(self.classifications(), {"old": "merged"})

    def test_rebased(self):
        self.make_unmerged()
        self.git("cherry-pick", "feature")
        self.assertEqual(self.classifications(), {"feature": "rebased"})

    def test_squash_merged(self):
        self.git("switch", "-qc", "feature")
        self.commit("first")
        self.commit("second")
        self.git("switch", "-q", "master")
        self.commit("main")
        self.git("merge", "--squash", "feature")
        self.git("commit", "-qm", "squash")
        self.assertEqual(self.classifications(), {"feature": "squash-merged"})

    def test_unmerged_and_quoted_names(self):
        self.make_unmerged("feature'quote")
        self.git("branch", 'feature"quote', "feature'quote")
        self.assertEqual(self.classifications(), {
            "feature'quote": "unmerged", 'feature"quote': "unmerged",
        })

    def test_conflict_is_unmerged(self):
        self.git("switch", "-qc", "feature")
        self.commit("root", "feature\n")
        self.git("switch", "-q", "master")
        self.commit("root", "master\n")
        self.assertEqual(self.classifications(), {"feature": "unmerged"})

    def test_content_added_in_merge_commit_is_unmerged(self):
        self.git("branch", "feature")
        self.commit("main")
        self.git("switch", "-q", "feature")
        self.git("merge", "--no-commit", "--no-ff", "master")
        self.commit("merge-only")
        self.git("switch", "-q", "master")
        self.assertEqual(self.git("cherry", "master", "feature"), "")
        self.assertEqual(self.classifications(), {"feature": "unmerged"})

    def test_tag_with_branch_name_does_not_change_candidate(self):
        self.make_unmerged()
        self.git("tag", "feature", "master")
        self.assertEqual(self.classifications(), {"feature": "unmerged"})

    def test_cherry_failure_stops_scan(self):
        self.make_unmerged()
        self.replace_cherry("sys.exit(128)")
        result = self.scan()
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("(rebased)", result.stdout)

    def test_large_cherry_output_is_not_lost_to_sigpipe(self):
        self.make_unmerged()
        self.env["SHELLOPTS"] = "pipefail"
        self.replace_cherry("for _ in range(10000):\n    os.write(1, b'+ ' + b'0' * 40 + b'\\n')\nsys.exit(0)")
        self.assertEqual(self.classifications(), {"feature": "unmerged"})

    def test_invalid_base_and_parallelism(self):
        self.assertNotEqual(self.scan("missing-base").returncode, 0)
        for parallel in ("0", "-1", "invalid"):
            with self.subTest(parallel=parallel):
                self.env["PARALLEL"] = parallel
                self.assertNotEqual(self.scan().returncode, 0)


if __name__ == "__main__":
    unittest.main()
