#!/usr/bin/env python3
"""Report translation keys defined in a Twine file that are no longer referenced
anywhere in the Organic Maps source tree.

This lives in the OM repository (not in the `tools/python/twine` submodule)
because "is this key used?" is a question about OM's own C++/Swift/Kotlin code
and resources, not about the Twine file format. The submodule is reused only as
a *library* to parse the Twine file into the set of defined keys.

Design notes:
- The scan is intentionally conservative: a false positive here can lead to
  deleting a string that is actually shown to users (e.g. an iOS permission
  prompt), so anything that might be referenced is treated as used.
- Keys built dynamically (e.g. `GetLocalizedString(colorName)`) cannot be found
  by a textual scan. List those in an allowlist file (one key per line, `#`
  comments allowed) passed via --allowlist.
"""

import argparse
import os
import re
import sys
from glob import iglob

# Reuse the Twine submodule purely as a parser for the .txt format.
_TWINE_PKG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "twine", "python_twine")
sys.path.insert(0, _TWINE_PKG)
try:
    from twine.twine_file import TwineFile
except ImportError as e:
    sys.exit(f"Cannot import Twine. Did you run 'git submodule update --init -- tools/python/twine'? ({e})")

# --- How OM code references a localized string by *literal* key, per source kind. ---
# C++: platform::GetLocalizedString("key"). Only the literal form is greppable;
# dynamic keys such as GetLocalizedString(reasonKey) must go in the allowlist.
_CORE_RE = re.compile(r'\bGetLocalizedString\(\s*"([^"]+)"')
# Android Java/Kotlin: R.string.key, R.plurals.key
_ANDROID_R_RE = re.compile(r'\bR\.(?:string|plurals)\.(\w+)')
# Android XML (layouts, menus, preferences, manifest): @string/key, @plurals/key
_ANDROID_XML_RE = re.compile(r'@(?:string|plurals)/(\w+)')
# iOS: every string literal inside an L(...) call. L() is `NSLocalizedString(str, nil)`,
# and `str` may be a ternary, e.g. L(flag ? "a" : "b") — so both keys must be captured.
_IOS_L_CALL_RE = re.compile(r'\bL\(([^)]*)\)')
_IOS_LITERAL_RE = re.compile(r'@?"(\w+)"')
# iOS: NSLocalizedString(@"key", @"comment") — only the first literal is the key.
_IOS_NS_RE = re.compile(r'\bNSLocalizedString\(\s*@?"(\w+)"')
# iOS Interface Builder runtime-localized attributes: value="key"
_IOS_XML_RE = re.compile(r'\bvalue="(\w+)"')
# iOS Info.plist declares system keys (NSLocation*, CFBundle*): <key>Key</key>.
# Their localized values are generated into InfoPlist.strings from the Twine file.
_IOS_PLIST_RE = re.compile(r'<key>(\w+)</key>')


def _regex_matcher(*regexes):
    """Build a text->set(keys) matcher that unions all capture groups of `regexes`."""
    def match(text):
        keys = set()
        for regex in regexes:
            keys.update(regex.findall(text))
        return keys
    return match


def _ios_source_matcher(text):
    keys = set()
    for call_args in _IOS_L_CALL_RE.findall(text):
        keys.update(_IOS_LITERAL_RE.findall(call_args))
    keys.update(_IOS_NS_RE.findall(text))
    return keys


# Per source root: list of (filename globs, text->set(keys) matcher).
_SCANNERS = {
    "core": [(("*.cpp", "*.hpp", "*.h", "*.mm"), _regex_matcher(_CORE_RE))],
    "android": [
        (("*.java", "*.kt"), _regex_matcher(_ANDROID_R_RE)),
        (("*.xml",), _regex_matcher(_ANDROID_XML_RE)),
    ],
    "ios": [
        (("*.swift", "*.m", "*.mm", "*.h"), _ios_source_matcher),
        (("*.xib", "*.storyboard"), _regex_matcher(_IOS_XML_RE)),
        (("*.plist",), _regex_matcher(_IOS_PLIST_RE)),
    ],
}


def scan_referenced_keys(root, scanners):
    """Return the set of keys referenced anywhere under `root`."""
    keys = set()
    for globs, matcher in scanners:
        for pattern in globs:
            for path in iglob(os.path.join(root, "**", pattern), recursive=True):
                # Read the whole file so calls spanning several lines are matched,
                # and decode leniently so one bad byte never skips a whole file.
                try:
                    with open(path, "r", encoding="utf-8", errors="ignore") as f:
                        text = f.read()
                except OSError:
                    continue  # unreadable file / broken symlink — skip, don't abort
                keys |= matcher(text)
    return keys


def load_allowlist(path):
    if not path:
        return set()
    allow = set()
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            key = line.split("#", 1)[0].strip()
            if key:
                allow.add(key)
    return allow


def subtract_allowlist(keys, allowlist):
    """Drop keys covered by the allowlist. An entry ending in '*' matches by prefix
    (whole dynamic families such as 'category_*' resolved via getIdentifier)."""
    exact = {a for a in allowlist if not a.endswith("*")}
    prefixes = tuple(a[:-1] for a in allowlist if a.endswith("*"))
    return {k for k in keys if k not in exact and not k.startswith(prefixes)}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("twine_file", help="Path to the Twine file (e.g. data/strings/strings.txt)")
    parser.add_argument("--core-src-root", help="C++ source root (e.g. libs)")
    parser.add_argument("--android-src-root", help="Android source root (e.g. android)")
    parser.add_argument("--ios-src-root", help="iOS source root (e.g. iphone)")
    parser.add_argument("--allowlist", help="File listing keys referenced dynamically (kept as used)")
    args = parser.parse_args(argv)

    if not any((args.core_src_root, args.android_src_root, args.ios_src_root)):
        parser.error("specify at least one of --core-src-root/--android-src-root/--ios-src-root")

    twine_file = TwineFile()
    twine_file.read(args.twine_file)
    defined = {d.key for section in twine_file.sections for d in section.definitions}

    referenced = set()
    for name, root in (("core", args.core_src_root),
                       ("android", args.android_src_root),
                       ("ios", args.ios_src_root)):
        if root:
            referenced |= scan_referenced_keys(root, _SCANNERS[name])

    allowlist = load_allowlist(args.allowlist)
    candidates = defined - referenced
    unused = sorted(subtract_allowlist(candidates, allowlist))

    print(f"Defined: {len(defined)}, referenced in code: {len(referenced & defined)}, "
          f"allowlisted: {len(candidates) - len(unused)}")
    if unused:
        print(f"Found {len(unused)} definitions/keys which are no longer used in the codebase:")
        print(*unused, sep="\n")
        return 1
    print("All good. There are no unused translation definitions/keys.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
