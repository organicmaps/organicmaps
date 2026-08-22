#!/usr/bin/env python3

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import format_json_xml as formatter


class FormatJsonTest(unittest.TestCase):
    def test_formats_exact_numbers(self):
        self.assertEqual(
            formatter.format_json('{"integer": 12345678901234567890, "exponent": 1e3}'),
            '{\n  "integer": 12345678901234567890,\n  "exponent": 1000.0\n}\n')

    def test_rejects_duplicate_object_names(self):
        with self.assertRaisesRegex(ValueError, "duplicate object name: value"):
            formatter.format_json('{"value": 1, "value": 2}')

    def test_rejects_precision_loss(self):
        with self.assertRaisesRegex(ValueError, "without changing its value"):
            formatter.format_json('{"value": 0.12345678901234567890123456789}')

    def test_rejects_overflow(self):
        with self.assertRaises(ValueError):
            formatter.format_json('{"value": 1e400}')

    def test_rejects_non_standard_constant(self):
        with self.assertRaisesRegex(ValueError, "invalid JSON constant: NaN"):
            formatter.format_json('{"value": NaN}')


class FormatXmlTest(unittest.TestCase):
    def _self_closing_with_length(self, length):
        fixed = len('<item first="1" second=""/>')
        return f'<item first="1" second="{"x" * (length - fixed)}"/>'

    def test_keeps_short_text_leaf_on_one_line(self):
        raw = (
            b'<string name="navigation_channel_name" translatable="false">'
            b'Navigation</string>')
        self.assertEqual(formatter.format_xml(raw), raw.decode() + "\n")

    def test_keeps_120_character_tag_on_one_line(self):
        xml = self._self_closing_with_length(formatter.MAX_LINE_LENGTH)
        self.assertEqual(formatter.format_xml(xml.encode()), xml + "\n")

    def test_wraps_121_character_tag(self):
        xml = self._self_closing_with_length(formatter.MAX_LINE_LENGTH + 1)
        value = "x" * (len(xml) - len('<item first="1" second=""/>'))
        expected = f'<item\n    first="1"\n    second="{value}"/>\n'
        self.assertEqual(formatter.format_xml(xml.encode()), expected)
        self.assertEqual(formatter.format_xml(expected.encode()), expected)

    def test_nested_indentation_counts_towards_line_length(self):
        child = self._self_closing_with_length(formatter.MAX_LINE_LENGTH - len(formatter.INDENT) + 1)
        xml = f"<root>{child}</root>"
        value = "x" * (len(child) - len('<item first="1" second=""/>'))
        expected = (
            f'<root>\n  <item\n      first="1"\n      second="{value}"/>\n</root>\n')
        self.assertEqual(formatter.format_xml(xml.encode()), expected)

    def test_preserves_whitespace_only_leaf(self):
        self.assertEqual(formatter.format_xml(b"<item> </item>"), "<item> </item>\n")

    def test_skips_cdata(self):
        with self.assertRaisesRegex(formatter.SkipFile, "CDATA"):
            formatter.format_xml(b"<item><![CDATA[a&b]]></item>")

    def test_process_skips_non_utf8_xml_before_decoding(self):
        with tempfile.TemporaryDirectory() as directory:
            old_repo = formatter.REPO
            formatter.REPO = Path(directory)
            try:
                Path(directory, "input.xml").write_bytes(
                    b'<?xml version="1.0" encoding="ISO-8859-1"?><item>\xe9</item>')
                self.assertEqual(
                    formatter.process("input.xml", fix=True),
                    "skip:non-utf-8 encoding")
            finally:
                formatter.REPO = old_repo

    def test_process_writes_utf8_with_lf(self):
        with tempfile.TemporaryDirectory() as directory:
            old_repo = formatter.REPO
            formatter.REPO = Path(directory)
            try:
                path = Path(directory, "input.json")
                path.write_bytes(b'{"value": 1}\r\n')
                self.assertEqual(formatter.process("input.json", fix=True), "changed")
                self.assertEqual(path.read_bytes(), b'{\n  "value": 1\n}\n')
            finally:
                formatter.REPO = old_repo


@unittest.skipUnless(shutil.which("bash") and shutil.which("git"), "requires bash and git")
class PreCommitHookTest(unittest.TestCase):
    def test_skipped_file_does_not_stage_unstaged_edits(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            hooks = repo / "tools/hooks"
            python = repo / "tools/python"
            hooks.mkdir(parents=True)
            python.mkdir(parents=True)
            shutil.copy2(formatter.REPO / "tools/hooks/pre-commit", hooks)
            shutil.copy2(formatter.REPO / "tools/hooks/format-config.bash", hooks)
            shutil.copy2(formatter.__file__, python)

            subprocess.run(["git", "init", "-q"], cwd=repo, check=True)
            rel = "iphone/App.xcassets/Icon.imageset/Contents.json"
            path = repo / rel
            path.parent.mkdir(parents=True)
            original = b'{"staged": false}\n'
            staged = b'{"staged": true}\n'
            unstaged = b'{"staged": true, "unstaged": true}\n'
            path.write_bytes(original)
            subprocess.run(["git", "add", str(path.relative_to(repo))], cwd=repo, check=True)
            subprocess.run(
                ["git", "-c", "user.name=Test", "-c", "user.email=test@example.com",
                 "commit", "-q", "-m", "Initial commit"],
                cwd=repo, check=True)
            path.write_bytes(staged)
            subprocess.run(["git", "add", str(path.relative_to(repo))], cwd=repo, check=True)
            path.write_bytes(unstaged)

            hook = subprocess.run(
                ["bash", "tools/hooks/pre-commit"], cwd=repo,
                capture_output=True, text=True)
            self.assertEqual(hook.returncode, 0, hook.stdout + hook.stderr)

            index = subprocess.run(
                ["git", "show", f":{rel}"], cwd=repo, check=True,
                capture_output=True).stdout
            self.assertEqual(index, staged)
            self.assertEqual(path.read_bytes(), unstaged)


if __name__ == "__main__":
    unittest.main()
