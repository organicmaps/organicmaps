#!/usr/bin/env python3
"""Exercise release version generation with real Git histories."""

import datetime
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


VERSION_SCRIPT = Path(__file__).resolve().parents[1] / "version.sh"
DAY = int(datetime.datetime(2026, 10, 17, 12, tzinfo=datetime.timezone.utc).timestamp())


class VersionTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.repo = Path(self.directory.name) / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")

    def git(self, *arguments, **kwargs):
        return subprocess.check_output(
            ["git", *arguments], cwd=self.repo, text=True, **kwargs
        ).strip()

    def history(self, dates):
        # fast-import builds large histories without one process per commit.
        records = []
        for timestamp, offset in dates:
            records.append(
                "commit refs/heads/main\n"
                f"committer Version Test <test@example.invalid> {timestamp} {offset}\n"
                "data 5\ntest\n\n"
            )
        self.git("fast-import", "--quiet", input="".join(records))
        return self.git("rev-list", "--reverse", "HEAD").splitlines()

    def version(self, kind="android_code", *, check=True, cwd=None):
        return subprocess.run(
            ["bash", str(VERSION_SCRIPT), kind],
            cwd=cwd or self.repo,
            env={**os.environ, "TZ": "HST10"},
            text=True,
            capture_output=True,
            check=check,
        )

    def test_counter_width_and_next_day_are_monotonic(self):
        commits = self.history([(DAY, "+0000")] * 129 + [(DAY + 86400, "+0000")])
        previous = 0
        for count, expected in [(99, 261017099), (100, 261017100),
                                (128, 261017128), (129, 261017129),
                                (130, 261018001)]:
            with self.subTest(commit=count):
                self.git("update-ref", "refs/heads/main", commits[count - 1])
                actual = int(self.version().stdout)
                self.assertEqual(actual, expected)
                self.assertGreater(actual, previous)
                self.assertLessEqual(actual + 1_000_000_000, 2_100_000_000)
                previous = actual

    def test_backdated_commit_does_not_hide_same_day_ancestors(self):
        self.history([(DAY, "+0000"), (DAY - 86400, "+0000"), (DAY, "+0000")])
        self.assertEqual(self.version().stdout.strip(), "261017002")
        self.assertEqual(self.version("ios_build").stdout.strip(), "2")

    def test_date_belongs_to_head(self):
        self.history([(DAY + 86400, "+0000"), (DAY, "+0000")])
        self.assertEqual(self.version().stdout.strip(), "261017001")
        self.assertEqual(self.version("windows_version").stdout.strip(), "2026.1017.1.0")

    def test_commit_day_is_utc_regardless_of_offsets(self):
        midnight = DAY + 12 * 3600
        self.history([(midnight - 300, "+0300"), (midnight + 300, "-0500")])
        self.assertEqual(self.version().stdout.strip(), "261018001")
        self.assertEqual(self.version("ios_version").stdout.strip(), "2026.10.18")

    def test_android_rejects_daily_counter_overflow(self):
        commits = self.history([(DAY, "+0000")] * 1000)
        self.git("update-ref", "refs/heads/main", commits[998])
        self.assertEqual(self.version().stdout.strip(), "261017999")
        self.git("update-ref", "refs/heads/main", commits[999])
        result = self.version(check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")
        self.assertIn("999", result.stderr)
        self.assertEqual(self.version("ios_build").stdout.strip(), "1000")

    def test_source_archive_uses_utc_date_and_zero_counter(self):
        archive = Path(self.directory.name) / "archive"
        archive.mkdir()
        before = datetime.datetime.now(datetime.timezone.utc).strftime("%y%m%d")
        actual = self.version(cwd=archive).stdout.strip()
        after = datetime.datetime.now(datetime.timezone.utc).strftime("%y%m%d")
        self.assertIn(actual, {before + "000", after + "000"})


if __name__ == "__main__":
    unittest.main()
