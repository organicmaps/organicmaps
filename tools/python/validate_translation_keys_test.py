#!/usr/bin/env python3
"""Unit tests for every supported localized-string reference form."""

import io
import os
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

import validate_translation_keys as v


class CoreMatcherTest(unittest.TestCase):
    def test_get_localized_string(self):
        self.assertEqual(v._core_source_matcher('GetLocalizedString("foo")'), {"foo":0})

    def test_two_calls_one_line(self):
        line = '{GetLocalizedString("ft"), GetLocalizedString("miles_per_hour")}'
        self.assertEqual(v._core_source_matcher(line), {"ft":0, "miles_per_hour":0})

    def test_multiline_call(self):
        self.assertEqual(
            v._core_source_matcher('GetLocalizedString(\n    "baz")'), {"baz": 1}
        )

    def test_category_literals_have_their_own_lines(self):
        source = (
            'DisplayedCategories::DisplayedCategories() {\n'
            '  m_keys = {"category_eat",\n'
            '            "category_hotel"};\n'
            '}\n'
        )
        self.assertEqual(
            v._category_source_matcher(source),
            {"category_eat": 1, "category_hotel": 2},
        )

    def test_strings_bundle_ternary_lines(self):
        source = 'bundle.GetString(condition ?\n    "core_entrance" :\n    "core_exit")'
        self.assertEqual(
            v._core_source_matcher(source), {"core_entrance": 1, "core_exit": 2}
        )

    def test_strings_bundle_literal(self):
        self.assertEqual(
            v._core_source_matcher('bundle.GetString("core_my_places")'),
            {"core_my_places":0},
        )

    def test_strings_bundle_ternary(self):
        line = 'bundle.GetString(isEntrance ? "core_entrance" : "core_exit")'
        self.assertEqual(v._core_source_matcher(line), {"core_entrance":0, "core_exit":0})

    def test_category_literals(self):
        line = (
            'DisplayedCategories::DisplayedCategories() {'
            'm_keys = {"category_eat", "category_hotel"};}'
        )
        self.assertEqual(
            v._core_source_matcher(line), {"category_eat":0, "category_hotel":0}
        )

    def test_category_matcher(self):
        line = (
            'DisplayedCategories::DisplayedCategories() {'
            'm_keys = {"category_eat", "category_hotel"};}'
        )
        self.assertEqual(
            v._category_source_matcher(line), {"category_eat":0, "category_hotel":0}
        )

    def test_unrelated_keys_member_is_ignored(self):
        line = 'Widget::Widget() {m_keys = {"not_a_translation", "also_not"};}'
        self.assertEqual(v._category_source_matcher(line), {})

    def test_unrelated_category_literal_is_ignored(self):
        self.assertEqual(v._core_source_matcher('Check("category_old")'), {})

    def test_dynamic_key_not_captured(self):
        self.assertEqual(v._core_source_matcher("GetLocalizedString(reasonKey)"), {})


class QtMatcherTest(unittest.TestCase):
    def test_literal_calls(self):
        cases = [
            ('Tr("app_exit")', {"app_exit": 0}),
            ('qt::Tr("version").arg("not_a_key")', {"version": 0}),
            ('::qt::Tr("version")', {"version": 0}),
            ('qt::Tr("count", n)', {"count": 0}),
            ('QT_TRID_NOOP("stored")', {"stored": 0}),
            ('QT_TRID_N_NOOP("plural")', {"plural": 0}),
            ('qt::Tr (\n  "count",\n  n)', {"count": 1}),
            ('Tr("first");\nqt::Tr("second")', {"first": 0, "second": 1}),
        ]
        for scanners in (v._SCANNERS, v._STRICT_SCANNERS):
            for source, expected in cases:
                with self.subTest(source=source, strict=scanners is v._STRICT_SCANNERS):
                    references = {}
                    for _, matcher in scanners["qt"]:
                        references.update(matcher(source))
                    self.assertEqual(references, expected)

    def test_nonliteral_and_unrelated_calls_are_ignored(self):
        for source in ('Tr(key)', 'qt::Tr(key)', 'tr("Source text")', 'SomeTr("not_a_key")'):
            for scanners in (v._SCANNERS, v._STRICT_SCANNERS):
                with self.subTest(source=source, strict=scanners is v._STRICT_SCANNERS):
                    for _, matcher in scanners["qt"]:
                        self.assertEqual(matcher(source), {})

    def test_qt_calls_are_not_core_references(self):
        self.assertEqual(v._core_source_matcher('Tr("first"); qt::Tr("second")'), {})


class QtValidationTest(unittest.TestCase):
    @staticmethod
    def _validate(definitions, sources):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("data/strings", "libs", "qt", "android", "iphone/Maps", "iphone/CoreApi"):
                (root / name).mkdir(parents=True, exist_ok=True)
            strings = "[[Strings]]\n\n" + "\n".join(
                f"[{key}]\ntags = {tags}\nen = Message\n" for key, tags in definitions.items()
            )
            (root / "data/strings/strings.txt").write_text(strings, encoding="utf-8")
            for name, source in sources.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source, encoding="utf-8")
            output = io.StringIO()
            with patch.object(v, "_REPO_ROOT", root), redirect_stdout(output):
                result = v.main()
        return result, output.getvalue()

    def test_qt_only_reference_needs_no_mobile_tags(self):
        result, output = self._validate({"qt_key": "qt"}, {"qt/widget.cpp": 'qt::Tr("qt_key")'})
        self.assertEqual(result, 0, output)

    def test_qt_reuses_mobile_translation_with_both_tags(self):
        result, output = self._validate(
            {"shared": "qt,android-app"},
            {"qt/widget.cpp": 'Tr("shared")', "android/Main.java": 'getString(R.string.shared)'},
        )
        self.assertEqual(result, 0, output)

    def test_qt_reference_requires_qt_tag(self):
        result, output = self._validate({"shared": ""}, {"qt/widget.cpp": 'Tr("shared")'})
        self.assertEqual(result, 1, output)
        self.assertIn("referenced by Qt but missing its consumer tag", output)
        self.assertIn("- shared (qt)", output)

    def test_shared_core_reference_requires_qt_tag(self):
        result, output = self._validate(
            {"shared": "android-sdk,apple-maps"},
            {"libs/core.cpp": 'platform::GetLocalizedString("shared")'},
        )
        self.assertEqual(result, 1, output)
        self.assertIn("- shared (qt)", output)

    def test_shared_core_reference_justifies_qt_tag(self):
        result, output = self._validate(
            {"shared": "qt,android-sdk,apple-maps"},
            {"libs/core.cpp": 'platform::GetLocalizedString("shared")'},
        )
        self.assertEqual(result, 0, output)

    def test_missing_qt_definition_reports_source_line(self):
        result, output = self._validate(
            {"live": "qt"},
            {"qt/widget.hpp": 'Tr("live");\nqt::Tr(\n  "missing", n);'},
        )
        self.assertEqual(result, 1, output)
        self.assertIn("referenced in code but missing from strings.txt", output)
        self.assertIn("- missing at ", output)
        self.assertIn("qt/widget.hpp:3", output)

    def test_unused_qt_definition_is_rejected(self):
        result, output = self._validate({"unused": "qt"}, {})
        self.assertEqual(result, 1, output)
        self.assertIn("no longer used in the codebase", output)
        self.assertIn("- unused", output)

    def test_qt_reference_does_not_justify_android_tag(self):
        result, output = self._validate(
            {"qt_key": "qt,android-app"}, {"qt/widget.cpp": 'Tr("qt_key")'},
        )
        self.assertEqual(result, 1, output)
        self.assertIn("- qt_key (android)", output)

    def test_qt_reference_does_not_justify_apple_tag(self):
        result, output = self._validate(
            {"qt_key": "qt,apple-maps"}, {"qt/widget.cpp": 'Tr("qt_key")'},
        )
        self.assertEqual(result, 1, output)
        self.assertIn("- qt_key (apple-maps)", output)

    def test_unrelated_tr_call_outside_qt_does_not_keep_key_alive(self):
        result, output = self._validate({"unused": "qt"}, {"libs/other.cpp": 'Tr("unused")'})
        self.assertEqual(result, 1, output)
        self.assertIn("no longer used in the codebase", output)


class IosMatcherTest(unittest.TestCase):
    def test_swift_and_objc_literal(self):
        self.assertEqual(v._ios_source_matcher('L("a")'), {"a":0})
        self.assertEqual(v._ios_source_matcher('L(@"b")'), {"b":0})

    def test_ternary_swift(self):
        line = 'L(category.isVisible ? "hide_from_map" : "zoom_to_country")'
        self.assertEqual(
            v._ios_source_matcher(line), {"hide_from_map": 0, "zoom_to_country": 0}
        )

    def test_ternary_objc(self):
        line = 'L(isApplying ? @"downloader_applying" : @"downloader_process")'
        self.assertEqual(
            v._ios_source_matcher(line), {"downloader_applying": 0, "downloader_process": 0}
        )

    def test_multiline_ternary(self):
        source = 'L(isVisible ?\n  "hide_from_map" :\n  "zoom_to_country")'
        self.assertEqual(
            v._ios_source_matcher(source), {"hide_from_map": 1, "zoom_to_country": 2}
        )

    def test_two_calls_one_line(self):
        line = (
            'CPListItem(text: L("not_all_shown_bookmarks_carplay"), '
            'detailText: L("switch_to_phone_bookmarks_carplay"))'
        )
        self.assertEqual(
            v._ios_source_matcher(line),
            {"not_all_shown_bookmarks_carplay": 0,
             "switch_to_phone_bookmarks_carplay": 0},
        )

    def test_nslocalizedstring_comment_not_captured(self):
        self.assertEqual(
            v._ios_source_matcher('NSLocalizedString(@"real_key", @"comment")'),
            {"real_key": 0},
        )

    def test_url_is_not_an_l_call(self):
        self.assertEqual(v._ios_source_matcher('let u = URL(string: "https")'), {})


class AndroidMatcherTest(unittest.TestCase):
    def test_r_string_java_kotlin(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(
            match("context.getString(R.string.color_picker_hex_label)"),
            {"color_picker_hex_label":0},
        )
        self.assertEqual(match("R.plurals.count"), {"count":0})

    def test_android_framework_resource_is_ignored(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(match("android.R.string.ok"), {})

    def test_r_string_ternary_both_branches(self):
        match = v._regex_matcher(v._ANDROID_R_RE)
        self.assertEqual(match("getString(b ? R.string.x : R.string.y)"), {"x":0, "y":0})

    def test_xml_string_refs(self):
        match = v._xml_matcher(v._ANDROID_XML_RE)
        self.assertEqual(match('android:text="@string/foo"'), {"foo":0})
        self.assertEqual(match("@plurals/bar"), {"bar":0})

    def test_xml_comments_are_ignored(self):
        match = v._xml_matcher(v._ANDROID_XML_RE)
        self.assertEqual(match('<!-- android:text="@string/unused" -->'), {})

    def test_xml_comments_preserve_reference_lines(self):
        match = v._xml_matcher(v._ANDROID_XML_RE)
        source = (
            '<!-- First line\n'
            '     Second line\n'
            '     android:text="@string/unused" -->\n'
            '<TextView android:text="@string/real"/>\n'
        )
        self.assertEqual(match(source), {"real": 3})


class PlistAndXibMatcherTest(unittest.TestCase):
    def test_plist_key(self):
        match = v._xml_matcher(v._IOS_PLIST_KEY_RE)
        self.assertEqual(
            match("<key>NSLocationWhenInUseUsageDescription</key>"),
            {"NSLocationWhenInUseUsageDescription": 0},
        )

    def test_plist_unrelated_key_is_ignored(self):
        match = v._xml_matcher(v._IOS_PLIST_KEY_RE)
        self.assertEqual(match("<key>CFBundleDisplayName</key>"), {})

    def test_plist_title_value(self):
        match = v._xml_matcher(v._IOS_PLIST_TITLE_RE)
        self.assertEqual(
            match("<key>UIApplicationShortcutItemTitle</key>\n<string>route</string>"),
            {"route": 1},
        )

    def test_multiline_localized_attribute(self):
        match = v._xml_matcher(v._IOS_XML_RE)
        source = (
            '<userDefinedRuntimeAttribute\n'
            '  keyPath="localizedText"\n'
            '  value="some_key"/>\n'
        )
        self.assertEqual(match(source), {"some_key": 2})

    def test_xib_localized_value(self):
        match = v._xml_matcher(v._IOS_XML_RE)
        attribute = '<userDefinedRuntimeAttribute type="string" keyPath="localizedText" value="some_key"/>'
        self.assertEqual(match(attribute), {"some_key": 0})

    def test_xib_unrelated_value_is_ignored(self):
        match = v._xml_matcher(v._IOS_XML_RE)
        self.assertEqual(
            match('<constraint firstAttribute="width" value="42"/>'), {}
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

            references = v.scan_referenced_keys(directory, v._SCANNERS["android"])
        self.assertEqual(references.keys(), {"kotlin_key", "xml_key"})

    def test_reports_the_original_file_and_line_after_xml_comments(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "Main.storyboard")
            with open(path, "w") as source:
                source.write(
                    '<!-- First line\n'
                    'Second line -->\n'
                    '<userDefinedRuntimeAttribute\n'
                    '  keyPath="localizedText" value="missing_key"/>\n'
                )
            references = v.scan_referenced_keys(directory, v._SCANNERS["ios"])
            with patch("builtins.print") as output:
                self.assertEqual(
                    v.report_references("missing from strings.txt", ["missing_key"], references),
                    1,
                )
            output.assert_any_call(f"- missing_key at {path}:4")

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
                keys |= matcher(text).keys()
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
        return {
            "core": {c: v.SourceReference("core", i) for (i,c) in enumerate(core)},
            "android": {a: v.SourceReference("android", i) for(i, a) in enumerate(android)},
            "ios": {ios: v.SourceReference("ios", i) for (i, ios) in enumerate(ios)},
        }

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
                {"k": {"android-app", "apple-maps"}}, referenced, {}
            ),
            [],
        )

    def test_android_reference_without_android_tag(self):
        referenced = self._referenced(android=["k"])
        self.assertEqual(
            v.find_undertagged({"k": {"apple-maps"}}, referenced, {}),
            ["k (android)"],
        )

    def test_conservative_core_reference_alone_demands_no_tag(self):
        referenced = self._referenced(core=["core_k"])
        self.assertEqual(v.find_undertagged({"core_k": {}}, referenced, {}), [])

    def test_strict_core_reference_demands_android_tag(self):
        referenced = self._referenced(core=["core_k"])
        self.assertEqual(
            v.find_undertagged({"core_k": {"apple-maps"}},
                               referenced,
                               {"core_k": v.SourceReference("core", 1)}),
            ["core_k (android)"],
        )


class TargetTagCoverageTest(unittest.TestCase):
    @staticmethod
    def _referenced(maps=(), chart=(), infoplist=()):
        return {
            "apple-maps": {m: v.SourceReference("maps", i) for (i,m) in enumerate(maps)},
            "apple-chart": {c: v.SourceReference("chart", i) for(i, c) in enumerate(chart)},
            "apple-infoplist": {info: v.SourceReference("chart", i) for (i, info) in enumerate(infoplist)},
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
