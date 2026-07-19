#!/usr/bin/env python3
"""Unit tests for validate_unused_strings.py.

Run: python3 tools/python/validate_unused_strings_test.py
Each case encodes a real reference pattern from the OM codebase that the previous
twine-based scanner got wrong (or that an earlier version of this script did).
"""

import os
import tempfile
import unittest

import validate_unused_strings as v


class CoreMatcherTest(unittest.TestCase):
    def setUp(self):
        self.match = v._regex_matcher(v._CORE_RE)

    def test_single(self):
        self.assertEqual(self.match('GetLocalizedString("foo")'), {"foo"})

    def test_two_calls_one_line(self):
        # The original twine bug: only the first match per line was captured.
        line = '{GetLocalizedString("ft"), GetLocalizedString("miles_per_hour")}'
        self.assertEqual(self.match(line), {"ft", "miles_per_hour"})

    def test_namespaced(self):
        self.assertEqual(self.match('platform::GetLocalizedString("bar")'), {"bar"})

    def test_multiline_call(self):
        self.assertEqual(self.match('GetLocalizedString(\n    "baz")'), {"baz"})

    def test_dynamic_key_not_captured(self):
        # Variable keys are unscannable by design; they belong in the allowlist.
        self.assertEqual(self.match("GetLocalizedString(reasonKey)"), set())


class IosMatcherTest(unittest.TestCase):
    def match(self, text):
        return v._ios_source_matcher(text)

    def test_swift_and_objc_literal(self):
        self.assertEqual(self.match('L("a")'), {"a"})
        self.assertEqual(self.match('L(@"b")'), {"b"})

    def test_ternary_swift(self):
        # The bug this script first introduced: only the first branch was captured.
        line = 'L(category.isVisible ? "hide_from_map" : "zoom_to_country")'
        self.assertEqual(self.match(line), {"hide_from_map", "zoom_to_country"})

    def test_ternary_objc(self):
        line = 'L(isApplying ? @"downloader_applying" : @"downloader_process")'
        self.assertEqual(self.match(line), {"downloader_applying", "downloader_process"})

    def test_two_calls_one_line(self):
        line = 'CPListItem(text: L("not_all_shown_bookmarks_carplay"), detailText: L("switch_to_phone_bookmarks_carplay"))'
        self.assertEqual(self.match(line), {"not_all_shown_bookmarks_carplay", "switch_to_phone_bookmarks_carplay"})

    def test_nslocalizedstring_comment_not_captured(self):
        # The 2nd NSLocalizedString arg is a comment, not a key.
        self.assertEqual(self.match('NSLocalizedString(@"real_key", @"comment")'), {"real_key"})

    def test_url_is_not_an_L_call(self):
        # `\bL\(` must not match inside identifiers like URL(...).
        self.assertEqual(self.match('let u = URL(string: "https")'), set())


class AndroidMatcherTest(unittest.TestCase):
    def test_r_string_java_kotlin(self):
        m = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(m("context.getString(R.string.color_picker_hex_label)"), {"color_picker_hex_label"})
        self.assertEqual(m("R.plurals.count"), {"count"})

    def test_r_string_ternary_both_branches(self):
        m = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(m("getString(b ? R.string.x : R.string.y)"), {"x", "y"})

    def test_xml_string_refs(self):
        m = v._regex_matcher(v._ANDROID_XML_RE)
        self.assertEqual(m('android:text="@string/foo"'), {"foo"})
        self.assertEqual(m("@plurals/bar"), {"bar"})


class PlistAndXibMatcherTest(unittest.TestCase):
    def test_plist_key(self):
        m = v._regex_matcher(v._IOS_PLIST_RE)
        self.assertEqual(m("<key>NSLocationWhenInUseUsageDescription</key>"), {"NSLocationWhenInUseUsageDescription"})

    def test_xib_value(self):
        m = v._regex_matcher(v._IOS_XML_RE)
        self.assertIn("some_key", m('<userDefinedRuntimeAttribute value="some_key"/>'))


class AllowlistTest(unittest.TestCase):
    def test_comments_and_blanks_ignored(self):
        with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
            f.write("# header comment\n\nred  # dynamic via getIdentifier\nblue\n   \n")
            path = f.name
        try:
            self.assertEqual(v.load_allowlist(path), {"red", "blue"})
        finally:
            os.unlink(path)

    def test_empty_path(self):
        self.assertEqual(v.load_allowlist(None), set())


class SubtractAllowlistTest(unittest.TestCase):
    def test_exact_match(self):
        self.assertEqual(v.subtract_allowlist({"a", "b", "c"}, {"b"}), {"a", "c"})

    def test_prefix_match(self):
        keys = {"category_bank", "category_water", "route", "color"}
        self.assertEqual(v.subtract_allowlist(keys, {"category_*"}), {"route", "color"})

    def test_empty_allowlist_drops_nothing(self):
        # str.startswith(()) is False, so an empty prefix tuple must not match.
        self.assertEqual(v.subtract_allowlist({"a", "b"}, set()), {"a", "b"})


class ScanTreeTest(unittest.TestCase):
    def test_walks_files_by_extension(self):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "a.kt"), "w") as f:
                f.write("val t = getString(R.string.kotlin_key)\n")
            sub = os.path.join(d, "res", "layout")
            os.makedirs(sub)
            with open(os.path.join(sub, "main.xml"), "w") as f:
                f.write('<TextView android:text="@string/xml_key"/>\n')
            # A non-scanned extension must be ignored.
            with open(os.path.join(d, "ignore.txt"), "w") as f:
                f.write("@string/should_not_be_found\n")
            keys = v.scan_referenced_keys(d, v._SCANNERS["android"])
        self.assertEqual(keys, {"kotlin_key", "xml_key"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
