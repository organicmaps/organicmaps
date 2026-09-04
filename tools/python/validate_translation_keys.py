#!/usr/bin/env python3
"""Check that data/strings/strings.txt and the source code agree on translation keys.

A definition nothing references is dead weight, its tags must include every OS
that references it, and a key the code asks for but no definition provides is
shown to the user raw. Apple tags are also checked against the exact generated
target because an incorrect tag still compiles but omits the runtime resource.

The scanner is owned by Organic Maps because it encodes the repository layout
and the project-specific C++, Qt, Android, and iOS localization accessors. The
unused-key scan deliberately over-approximates references so it cannot suggest
deleting a live string. Missing-key and exact-target checks use only reference
forms whose ownership is unambiguous. Android module ownership remains a build
check because application resources may intentionally come from Gradle dependencies.

types_strings.txt and sound.txt primarily serve runtime-built feature-type and
turn-direction keys. Their catalogs are covered by format and regeneration checks;
this usage scan covers only strings.txt.
"""

import os
import re
import sys
from pathlib import Path
from dataclasses import dataclass

_REPO_ROOT = Path(__file__).resolve().parents[2]

# How Organic Maps source code references localized strings by literal key.
_CORE_RE = re.compile(r'\bGetLocalizedString\(\s*"([^"]+)"')
# StringsBundle::GetString() may receive a ternary expression, so every literal in the
# call is a possible key. Matching the bare name also catches unrelated GetString()
# overloads; this conservative matcher is used only to avoid unsafe deletion advice.
_CORE_BUNDLE_CALL_RE = re.compile(r"\bGetString\(([^)]*)\)")
_CORE_LITERAL_RE = re.compile(r'"([A-Za-z0-9_]+)"')
# DisplayedCategories owns keys that Android and iOS resolve dynamically.
_CATEGORY_LIST_RE = re.compile(
    r"DisplayedCategories::DisplayedCategories\b.*?\bm_keys\s*=\s*\{([^}]*)\}",
    re.DOTALL,
)
# Exclude explicit references to Android framework resources.
_ANDROID_R_RE = re.compile(r"(?<!android\.)\bR\.(?:string|plurals)\.(\w+)")
_ANDROID_XML_RE = re.compile(r"@(?:string|plurals)/(\w+)")
# L() may receive a ternary expression, so every literal in the call is a key.
_IOS_L_CALL_RE = re.compile(r"\bL\(([^)]*)\)")
_IOS_LITERAL_RE = re.compile(r'@?"(\w+)"')
_IOS_NS_RE = re.compile(r'\bNSLocalizedString\(\s*@?"(\w+)"')
_IOS_XML_RE = re.compile(
    r"<userDefinedRuntimeAttribute\b"
    r'(?=[^>]*\bkeyPath="localized(?:Text|Placeholder)")'
    r'(?=[^>]*\bvalue="(\w+)")[^>]*/?>'
)
# Info.plist localizes both key names (the usage-description family) and some values
# (a shortcut item's title is itself a key looked up in InfoPlist.strings).
_IOS_PLIST_KEY_RE = re.compile(r"<key>(\w*UsageDescription)</key>")
_IOS_PLIST_TITLE_RE = re.compile(
    r"<key>UIApplicationShortcutItemTitle</key>\s*<string>(\w+)</string>"
)
_XML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)


@dataclass
class SourceReference:
    filename: str
    line_number: int


def _match_references(text, regex, literal_regex=None) -> dict[str, int]:
    """Return keys and their zero-based lines, optionally extracting literals from a call or list."""
    references = {}
    line_number = 0
    previous_position = 0
    for match in regex.finditer(text):
        if literal_regex is None:
            keys = [(match.group(1), match.start(1))]
        else:
            keys = [
                (literal.group(1), match.start(1) + literal.start(1))
                for literal in literal_regex.finditer(match.group(1))
            ]
        for key, position in keys:
            # Matches are ordered, so count each interval once instead of rescanning prefixes.
            line_number += text.count("\n", previous_position, position)
            previous_position = position
            references[key] = line_number
    return references


def _regex_matcher(*regexes):
    """Build a matcher that combines key references from each regex's first capture group."""

    def match(text) -> dict[str, int]:
        references = {}
        for regex in regexes:
            references.update(_match_references(text, regex))
        return references

    return match


def _xml_matcher(*regexes):
    match = _regex_matcher(*regexes)

    def match_xml(text) -> dict[str, int]:
        # Keep the original line numbers when discarding commented-out references.
        return match(_XML_COMMENT_RE.sub(lambda m: "\n" * m.group(0).count("\n"), text))

    return match_xml


def _core_source_matcher(text) -> dict[str, int]:
    references = _match_references(text, _CORE_RE)
    references.update(_category_source_matcher(text))
    references.update(_match_references(text, _CORE_BUNDLE_CALL_RE, _CORE_LITERAL_RE))
    return references


def _category_source_matcher(text) -> dict[str, int]:
    return _match_references(text, _CATEGORY_LIST_RE, _CORE_LITERAL_RE)


def _ios_source_matcher(text) -> dict[str, int]:
    references = _match_references(text, _IOS_L_CALL_RE, _IOS_LITERAL_RE)
    references.update(_match_references(text, _IOS_NS_RE))
    return references


def _raise_os_error(error):
    """Fail the scan: a skipped directory would shrink the referenced set instead,
    turning a string that is still in use into a deletion candidate."""
    raise error


# Per source root: (filename extensions, text -> {key: zero-based line} matcher).
_IOS_RESOURCE_SCANNERS = [
    ((".swift", ".m", ".mm", ".h"), _ios_source_matcher),
    ((".xib", ".storyboard"), _xml_matcher(_IOS_XML_RE)),
]
_IOS_INFOPLIST_SCANNERS = [
    ((".plist",), _xml_matcher(_IOS_PLIST_KEY_RE, _IOS_PLIST_TITLE_RE))
]
_SCANNERS = {
    "core": [((".cpp", ".hpp", ".h", ".mm"), _core_source_matcher)],
    "android": [
        ((".java", ".kt"), _regex_matcher(_ANDROID_R_RE)),
        ((".xml",), _xml_matcher(_ANDROID_XML_RE)),
    ],
    "ios": _IOS_RESOURCE_SCANNERS + _IOS_INFOPLIST_SCANNERS,
}

# Which source tree each scanner reads.
_ROOTS = (("core", "libs"), ("core", "qt"), ("android", "android"), ("ios", "iphone"))

# Apple code and resources have unambiguous generated localization targets. Core
# references belong to the Maps app. Chart has no localization lookups or resources,
# so its tag deliberately has no source roots.
_APPLE_TARGETS = ("apple-maps", "apple-chart", "apple-infoplist")
_APPLE_TARGET_ROOTS = (
    ("apple-maps", "iphone/Maps", _IOS_RESOURCE_SCANNERS),
    ("apple-maps", "iphone/CoreApi", _IOS_RESOURCE_SCANNERS),
    ("apple-infoplist", "iphone/Maps", _IOS_INFOPLIST_SCANNERS),
)

# Reference forms whose literal is always a strings.txt key. The scanners above cannot
# answer the opposite question because they deliberately match more than strings.txt:
# Android resources also come from donottranslate.xml and the framework; Info.plist
# usage descriptions can be provided inline without a translation definition.
# Over-matching only keeps a dead string alive, but here it would demand definitions
# for keys that rightly live elsewhere.
_STRICT_SCANNERS = {
    "core": [
        ((".cpp", ".hpp", ".h", ".mm"), _regex_matcher(_CORE_RE)),
        ((".cpp",), _category_source_matcher),
    ],
    "ios": _IOS_RESOURCE_SCANNERS
    + [
        ((".plist",), _xml_matcher(_IOS_PLIST_TITLE_RE)),
    ],
}


def scan_referenced_keys(root, scanners) -> dict[str, SourceReference]:
    """Return dictionary {keys: source_reference} for keys referenced under root, excluding generated build trees."""
    references = {}
    for directory, subdirectories, filenames in os.walk(root, onerror=_raise_os_error):
        subdirectories[:] = [name for name in subdirectories if name != "build"]
        for filename in filenames:
            matchers = [
                matcher
                for extensions, matcher in scanners
                if filename.endswith(extensions)
            ]
            if not matchers:
                continue

            path = os.path.join(directory, filename)
            with open(path, "r", encoding="utf-8") as source:
                text = source.read()
            for matcher in matchers:
                references.update(
                    {key: SourceReference(path, line_number) for key, line_number in matcher(text).items()}
                )
    return references


def scan_all(scanners) -> dict[str, dict[str, SourceReference]]:
    """Return {scanner_name: {key: source_reference}}."""
    referenced = {name: {} for name in scanners}
    for name, relative_root in _ROOTS:
        if name in scanners:
            referenced[name] |= scan_referenced_keys(
                _REPO_ROOT / relative_root, scanners[name]
            )
    return referenced


def scan_apple_targets(core_referenced) -> dict[str, dict[str, SourceReference]]:
    """Return references grouped by the Apple tag that generates their resource."""
    referenced = {tag: {} for tag in _APPLE_TARGETS}
    for tag, relative_root, scanners in _APPLE_TARGET_ROOTS:
        referenced[tag] |= scan_referenced_keys(_REPO_ROOT / relative_root, scanners)
    referenced["apple-maps"] |= core_referenced
    return referenced


def find_overtagged(tags_by_key, referenced) -> list[str]:
    """Return "key (android)" for every Android tag with no app or core reference."""
    # Core lookups justify Android tags; module ownership is checked by the Android build.
    return sorted(
        f"{key} (android)"
        for key, tags in tags_by_key.items()
        if any(tag.startswith("android") for tag in tags)
        and key not in referenced["android"]
        and key not in referenced["core"]
    )


def find_undertagged(tags_by_key, referenced, core_referenced) -> list[str]:
    """Return "key (android)" for every required Android tag that is absent.

    Such a key is missing from that platform's generated resources, so the lookup
    finds nothing at runtime even though the definition exists. Strict core
    references require Android resources because the core code runs in that app.
    """
    return sorted(
        f"{key} (android)"
        for key in (referenced["android"].keys() | core_referenced.keys()) & tags_by_key.keys()
        if not any(tag.startswith("android") for tag in tags_by_key[key])
    )


def find_overtagged_targets(tags_by_key, referenced_by_tag) -> list[str]:
    """Return "key (tag)" for every exact target tag with no reference."""
    return sorted(
        f"{key} ({tag})"
        for key, tags in tags_by_key.items()
        for tag in tags & referenced_by_tag.keys()
        if key not in referenced_by_tag[tag]
    )


def find_undertagged_targets(tags_by_key, referenced_by_tag):
    """Return "key (tag)" for every exact target reference its tags leave out."""
    return sorted(
        f"{key} ({tag})"
        for tag, referenced in referenced_by_tag.items()
        for key in referenced & tags_by_key.keys()
        if tag not in tags_by_key[key]
    )


def report(problem: str, keys: list[str]) -> int:
    """Print the offending keys, if any, and return the number of failed checks."""
    if not keys:
        return 0
    print(f"Found {len(keys)} translation keys {problem}:")
    print("- ", end="")
    print(*keys, sep="\n- ")
    return 1


def report_references(problem: str, keys: list[str], source_refs: dict[str, SourceReference]) -> int:
    """Print the offending keys with source references, if any, and return the number of failed checks."""
    if not keys:
        return 0
    print(f"Found {len(keys)} translation keys {problem}:")
    for k in keys:
        ref = source_refs[k]
        print(f"- {k} at {ref.filename}:{ref.line_number+1}")
    return 1


def main():
    sys.path.insert(0, str(Path(__file__).resolve().parent / "twine" / "python_twine"))
    try:
        from twine.twine_file import TwineFile
    except ImportError as error:
        sys.exit(
            f"Cannot import Twine. Did you run 'git submodule update --init -- tools/python/twine'? ({error})"
        )

    twine_file = TwineFile()
    twine_file.read(_REPO_ROOT / "data" / "strings" / "strings.txt")
    tags_by_key = {
        key: set(definition.tags or ())
        for key, definition in twine_file.definitions_by_key.items()
    }

    referenced = scan_all(_SCANNERS)
    strict_referenced = scan_all(_STRICT_SCANNERS)
    used = set().union(*referenced.values())
    print(f"Defined: {len(tags_by_key)}, referenced in code: {len(used & set(tags_by_key))}")

    failures = report("no longer used in the codebase", sorted(set(tags_by_key) - used))
    failures += report(
        "tagged for a platform that never references them",
        find_overtagged(tags_by_key, referenced),
    )
    failures += report(
        "referenced by a platform their tags leave out",
        find_undertagged(tags_by_key, referenced, strict_referenced["core"]),
    )
    apple_referenced = scan_apple_targets(strict_referenced["core"])
    failures += report(
        "tagged for an Apple target that never references them",
        find_overtagged_targets(tags_by_key, apple_referenced),
    )
    failures += report(
        "referenced by an Apple target their tags leave out",
        find_undertagged_targets(tags_by_key, apple_referenced),
    )
    strict = strict_referenced["ios"] | strict_referenced["core"]
    failures += report_references(
        "referenced in code but missing from strings.txt",
        sorted(strict.keys() - set(tags_by_key)),
        strict,
    )
    if failures:
        return 1
    print("All good. Translation keys and their tags match the code.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
