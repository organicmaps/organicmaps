#!/usr/bin/env python3
"""Unit tests for every supported localized-string reference form."""

import os
import tempfile
import unittest
from unittest.mock import patch

import validate_translation_keys as v


class CoreMatcherTest(unittest.TestCase):
    def test_get_localized_string(self):
        self.assertEqual(v._core_source_matcher('GetLocalizedString("foo")'), {"foo"})

    def test_two_calls_one_line(self):
        line = '{GetLocalizedString("ft"), GetLocalizedString("miles_per_hour")}'
        self.assertEqual(v._core_source_matcher(line), {"ft", "miles_per_hour"})

    def test_multiline_call(self):
        self.assertEqual(
            v._core_source_matcher('GetLocalizedString(\n    "baz")'), {"baz"}
        )

    def test_strings_bundle_literal(self):
        self.assertEqual(
            v._core_source_matcher('bundle.GetString("core_my_places")'),
            {"core_my_places"},
        )

    def test_strings_bundle_ternary(self):
        line = 'bundle.GetString(isEntrance ? "core_entrance" : "core_exit")'
        self.assertEqual(v._core_source_matcher(line), {"core_entrance", "core_exit"})

    def test_category_literals(self):
        line = (
            'DisplayedCategories::DisplayedCategories() {'
            'm_keys = {"category_eat", "category_hotel"};}'
        )
        self.assertEqual(
            v._core_source_matcher(line), {"category_eat", "category_hotel"}
        )

    def test_category_matcher(self):
        line = (
            'DisplayedCategories::DisplayedCategories() {'
            'm_keys = {"category_eat", "category_hotel"};}'
        )
        self.assertEqual(
            v._category_source_matcher(line), {"category_eat", "category_hotel"}
        )

    def test_unrelated_keys_member_is_ignored(self):
        line = 'Widget::Widget() {m_keys = {"not_a_translation", "also_not"};}'
        self.assertEqual(v._category_source_matcher(line), set())

    def test_unrelated_category_literal_is_ignored(self):
        self.assertEqual(v._core_source_matcher('Check("category_old")'), set())

    def test_dynamic_key_not_captured(self):
        self.assertEqual(v._core_source_matcher("GetLocalizedString(reasonKey)"), set())


class IosMatcherTest(unittest.TestCase):
    def test_swift_and_objc_literal(self):
        self.assertEqual(v._ios_source_matcher('L("a")'), {"a"})
        self.assertEqual(v._ios_source_matcher('L(@"b")'), {"b"})

    def test_ternary_swift(self):
        line = 'L(category.isVisible ? "hide_from_map" : "zoom_to_country")'
        self.assertEqual(
            v._ios_source_matcher(line), {"hide_from_map", "zoom_to_country"}
        )

    def test_ternary_objc(self):
        line = 'L(isApplying ? @"downloader_applying" : @"downloader_process")'
        self.assertEqual(
            v._ios_source_matcher(line), {"downloader_applying", "downloader_process"}
        )

    def test_two_calls_one_line(self):
        line = (
            'CPListItem(text: L("not_all_shown_bookmarks_carplay"), '
            'detailText: L("switch_to_phone_bookmarks_carplay"))'
        )
        self.assertEqual(
            v._ios_source_matcher(line),
            {"not_all_shown_bookmarks_carplay", "switch_to_phone_bookmarks_carplay"},
        )

    def test_nslocalizedstring_comment_not_captured(self):
        self.assertEqual(
            v._ios_source_matcher('NSLocalizedString(@"real_key", @"comment")'),
            {"real_key"},
        )

    def test_url_is_not_an_l_call(self):
        self.assertEqual(v._ios_source_matcher('let u = URL(string: "https")'), set())


class AndroidMatcherTest(unittest.TestCase):
    def test_r_string_java_kotlin(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(
            match("context.getString(R.string.color_picker_hex_label)"),
            {"color_picker_hex_label"},
        )
        self.assertEqual(match("R.plurals.count"), {"count"})

    def test_android_framework_resource_is_ignored(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(match("android.R.string.ok"), set())

    def test_r_string_ternary_both_branches(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(match("getString(b ? R.string.x : R.string.y)"), {"x", "y"})

    def test_xml_string_refs(self):
        match = v._xml_matcher(v._ANDROID_XML_RE)
        self.assertEqual(match('android:text="@string/foo"'), {"foo"})
        self.assertEqual(match("@plurals/bar"), {"bar"})

    def test_xml_comments_are_ignored(self):
        match = v._xml_matcher(v._ANDROID_XML_RE)
        self.assertEqual(match('<!-- android:text="@string/unused" -->'), set())


class PlistAndXibMatcherTest(unittest.TestCase):
    def test_plist_key(self):
        match = v._xml_matcher(v._IOS_PLIST_KEY_RE)
        self.assertEqual(
            match("<key>NSLocationWhenInUseUsageDescription</key>"),
            {"NSLocationWhenInUseUsageDescription"},
        )

    def test_plist_unrelated_key_is_ignored(self):
        match = v._xml_matcher(v._IOS_PLIST_KEY_RE)
        self.assertEqual(match("<key>CFBundleDisplayName</key>"), set())

    def test_plist_title_value(self):
        match = v._xml_matcher(v._IOS_PLIST_TITLE_RE)
        self.assertEqual(
            match("<key>UIApplicationShortcutItemTitle</key>\n<string>route</string>"),
            {"route"},
        )

    def test_xib_localized_value(self):
        match = v._xml_matcher(v._IOS_XML_RE)
        attribute = '<userDefinedRuntimeAttribute type="string" keyPath="localizedText" value="some_key"/>'
        self.assertEqual(match(attribute), {"some_key"})

    def test_xib_unrelated_value_is_ignored(self):
        match = v._xml_matcher(v._IOS_XML_RE)
        self.assertEqual(
            match('<constraint firstAttribute="width" value="42"/>'), set()
        )


class ScanTreeTest(unittest.TestCase):
    def test_walks_files_once_and_excludes_build_output(self):
        with tempfile.TemporaryDirectory() as directory:
            with open(os.path.join(directory, "a.kt"), "w") as source:
                source.write("val t = getString(R.string.kotlin_key)\n")

            resources = os.path.join(directory, "res", "layout")
            os.makedirs(resources)
            with open(os.path.join(resources, "main.xml"), "w") as source:
                source.write('<TextView android:text="@string/xml_key"/>\n')

            build = os.path.join(directory, "build")
            os.makedirs(build)
            with open(os.path.join(build, "stale.kt"), "w") as source:
                source.write("R.string.stale_key\n")

            with open(os.path.join(directory, "ignore.txt"), "w") as source:
                source.write("@string/ignored_extension\n")

            keys = v.scan_referenced_keys(directory, v._SCANNERS["android"])
        self.assertEqual(keys, {"kotlin_key", "xml_key"})

    def test_read_errors_fail_the_scan(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "source.kt")
            with open(path, "w") as source:
                source.write("R.string.key\n")
            with patch("builtins.open", side_effect=OSError("unreadable")):
                with self.assertRaises(OSError):
                    v.scan_referenced_keys(directory, v._SCANNERS["android"])

    def test_directory_errors_fail_the_scan(self):
        with patch("os.scandir", side_effect=OSError("unreadable")):
            with self.assertRaises(OSError):
                v.scan_referenced_keys("missing", v._SCANNERS["android"])

    def test_invalid_utf8_fails_the_scan(self):
        with tempfile.TemporaryDirectory() as directory:
            with open(os.path.join(directory, "source.kt"), "wb") as source:
                source.write(b"\xff")
            with self.assertRaises(UnicodeDecodeError):
                v.scan_referenced_keys(directory, v._SCANNERS["android"])


class StrictMatcherTest(unittest.TestCase):
    """The undefined-key direction must only accept literals that are always keys."""

    @staticmethod
    def _match(scanner_name, extension, text):
        keys = set()
        for extensions, matcher in v._STRICT_SCANNERS[scanner_name]:
            if extension in extensions:
                keys |= matcher(text)
        return keys

    def test_core_accepts_only_the_localization_accessor(self):
        self.assertEqual(self._match("core", ".cpp", 'GetLocalizedString("k")'), {"k"})
        # GetString() is also an unrelated overload on other classes.
        self.assertEqual(self._match("core", ".cpp", 'x.GetString("lang")'), set())

    def test_core_accepts_dynamic_category_definitions(self):
        line = (
            'DisplayedCategories::DisplayedCategories() {'
            'm_keys = {"category_bank", "category_children"};}'
        )
        self.assertEqual(
            self._match("core", ".cpp", line),
            {"category_bank", "category_children"},
        )

    def test_android_is_not_scanned(self):
        # R.string may resolve to donottranslate.xml or a framework resource.
        self.assertNotIn("android", v._STRICT_SCANNERS)

    def test_plist_accepts_the_shortcut_title_only(self):
        plist = (
            "<key>NSLocationWhenInUseUsageDescription</key>\n"
            "<key>UIApplicationShortcutItemTitle</key>\n<string>route</string>"
        )
        self.assertEqual(self._match("ios", ".plist", plist), {"route"})

    def test_ios_accepts_localized_storyboard_attributes(self):
        attribute = '<userDefinedRuntimeAttribute keyPath="localizedText" value="key"/>'
        self.assertEqual(self._match("ios", ".storyboard", attribute), {"key"})


class TagCoverageTest(unittest.TestCase):
    @staticmethod
    def _referenced(core=(), android=(), ios=()):
        return {"core": set(core), "android": set(android), "ios": set(ios)}

    def test_android_reference_justifies_android_tag(self):
        referenced = self._referenced(android=["k"])
        self.assertEqual(v.find_overtagged({"k": {"android-app", "apple-maps"}}, referenced), [])

    def test_android_tag_without_android_reference(self):
        referenced = self._referenced(ios=["k"])
        self.assertEqual(
            v.find_overtagged({"k": {"android-app", "apple-maps"}}, referenced), ["k (android)"]
        )

    def test_core_reference_justifies_android_tag(self):
        referenced = self._referenced(core=["core_k"])
        self.assertEqual(
            v.find_overtagged({"core_k": {"android-sdk", "apple-maps"}}, referenced), []
        )

    def test_untagged_definition_promises_nothing(self):
        self.assertEqual(v.find_overtagged({"k": set()}, self._referenced()), [])

    def test_android_tag_covers_android_reference(self):
        referenced = self._referenced(android=["k"])
        self.assertEqual(
            v.find_undertagged(
                {"k": {"android-app", "apple-maps"}}, referenced, set()
            ),
            [],
        )

    def test_android_reference_without_android_tag(self):
        referenced = self._referenced(android=["k"])
        self.assertEqual(
            v.find_undertagged({"k": {"apple-maps"}}, referenced, set()),
            ["k (android)"],
        )

    def test_conservative_core_reference_alone_demands_no_tag(self):
        referenced = self._referenced(core=["core_k"])
        self.assertEqual(v.find_undertagged({"core_k": set()}, referenced, set()), [])

    def test_strict_core_reference_demands_android_tag(self):
        referenced = self._referenced(core=["core_k"])
        self.assertEqual(
            v.find_undertagged({"core_k": {"apple-maps"}}, referenced, {"core_k"}),
            ["core_k (android)"],
        )


class TargetTagCoverageTest(unittest.TestCase):
    @staticmethod
    def _referenced(maps=(), chart=(), infoplist=()):
        return {
            "apple-maps": set(maps),
            "apple-chart": set(chart),
            "apple-infoplist": set(infoplist),
        }

    def test_exact_target_tags(self):
        tags = {
            "maps_key": {"apple-maps"},
            "chart_key": {"apple-chart"},
            "plist_key": {"apple-infoplist"},
        }
        referenced = self._referenced(
            maps=["maps_key"], chart=["chart_key"], infoplist=["plist_key"]
        )
        self.assertEqual(v.find_overtagged_targets(tags, referenced), [])
        self.assertEqual(v.find_undertagged_targets(tags, referenced), [])

    def test_wrong_target_is_reported_in_both_directions(self):
        tags = {"collections": {"apple-chart"}}
        referenced = self._referenced(maps=["collections"])
        self.assertEqual(
            v.find_overtagged_targets(tags, referenced),
            ["collections (apple-chart)"],
        )
        self.assertEqual(
            v.find_undertagged_targets(tags, referenced),
            ["collections (apple-maps)"],
        )


class ReportTest(unittest.TestCase):
    def test_no_keys_is_not_a_failure(self):
        self.assertEqual(v.report("unused", []), 0)

    def test_keys_are_a_failure(self):
        with patch("builtins.print"):
            self.assertEqual(v.report("unused", ["a", "b"]), 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
