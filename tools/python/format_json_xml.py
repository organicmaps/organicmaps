#!/usr/bin/env python3
#
# Format (or check) hand-edited JSON and XML files.
#
# Cross-platform and dependency-free: uses only the Python 3 standard library,
# so it runs identically in the pre-commit hook and in CI without installing a
# Node/prettier toolchain.
#
# The set of files that are formatted is defined by INCLUDE / EXCLUDE below --
# the single source of truth shared by the hook and CI (both call this script).
#
# JSON is always reformatted (stdlib json is lossless and order-preserving).
# XML is reflowed Android-style: an element with more than one attribute (xml
# namespaces count as attributes) puts the tag name alone on the opening line
# and every attribute on its own 4-space-indented line. This is only done when
# it is safe: files with mixed content (text interleaved with
# elements), xml:space="preserve", a DOCTYPE, or a non-UTF-8 encoding are left
# untouched, because reflowing them could change their meaning.
#
# Usage:
#   format_json_xml.py all               # check every in-scope file; exit 1 on drift
#   format_json_xml.py all --fix         # reformat every in-scope file
#   format_json_xml.py files A B ...      # check the given files
#   format_json_xml.py files A B ... --fix
#

import argparse
import json
import re
import subprocess
import sys
from fnmatch import fnmatch
from pathlib import Path
from xml.dom import Node, minidom

REPO = Path(__file__).resolve().parents[2]

INDENT = "  "
ATTR_INDENT = INDENT * 2  # continuation indent for wrapped attributes (4 spaces)

# --- scope: the single source of truth --------------------------------------
# Only files whose repo-relative POSIX path starts with one of these prefixes
# (or matches it exactly) are considered.
INCLUDE = [
    "android",                     # Android resource XML (layouts, drawables, values, ...)
    "data/conf",                   # hand-edited generator configs
    "data/skipped_elements.json",
    "package.json",
]

# fnmatch globs (vs the repo-relative POSIX path) that are never formatted,
# even when under an INCLUDE prefix. These are tool-generated or tool-owned.
EXCLUDE = [
    "*/res/values*/*strings.xml",       # twine-generated: strings/types_strings/wear_strings
    "*/build/*",                        # build artifacts
    "data/symbols/*",                   # skin generator output
    "*/countries.json",                 # map generator output (1-space indent; also android assets copy)
    "data/taginfo.json",                # tools/python/generate_taginfo.py output
    "data/*-strings/*/localize.json",   # twine JSON output
    "iphone/*Contents.json",            # Xcode-owned .xcassets
    "3party/*",
]


class SkipFile(Exception):
    """Raised when a file is well-formed but unsafe to reflow; left untouched."""


def in_scope(rel):
    if not rel.endswith((".json", ".xml")):
        return False
    if not any(rel == p or rel.startswith(p + "/") or fnmatch(rel, p) for p in INCLUDE):
        return False
    return not any(fnmatch(rel, pat) for pat in EXCLUDE)


# --- JSON -------------------------------------------------------------------
def format_json(text):
    # object_pairs_hook=dict preserves key order (dicts are ordered); indent=2
    # and ensure_ascii=False keep Cyrillic/CJK literal instead of \uXXXX escapes.
    data = json.loads(text)
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


# --- XML --------------------------------------------------------------------
_DECL_RE = re.compile(rb"^\s*<\?xml\b[^>]*\?>")
_ENC_RE = re.compile(rb'encoding\s*=\s*["\']([^"\']+)["\']')

_STRUCTURAL = (
    Node.ELEMENT_NODE,
    Node.COMMENT_NODE,
    Node.CDATA_SECTION_NODE,
    Node.PROCESSING_INSTRUCTION_NODE,
)


def _esc_text(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _esc_attr(s):
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace('"', "&quot;")
    return s.replace("\r", "&#13;").replace("\n", "&#10;").replace("\t", "&#9;")


def _attrs(el):
    a = el.attributes
    return [(a.item(i).name, a.item(i).value) for i in range(a.length)]


def _is_unsafe(node):
    """True if any element in the subtree cannot be reflowed without risk."""
    if node.nodeType == Node.ELEMENT_NODE:
        if node.getAttribute("xml:space") == "preserve":
            return True
        sig_text = [c for c in node.childNodes
                    if c.nodeType == Node.TEXT_NODE and c.data.strip()]
        structural = [c for c in node.childNodes if c.nodeType in _STRUCTURAL]
        # Mixed content: significant text alongside elements/comments/CDATA.
        if sig_text and structural:
            return True
        if len(sig_text) > 1:
            return True
    return any(_is_unsafe(c) for c in node.childNodes)


def _start_tag(name, attrs, pad, self_close):
    tail = "/>" if self_close else ">"
    # 0 or 1 attribute: always a single line.
    if len(attrs) <= 1:
        joined = "".join(f' {k}="{_esc_attr(v)}"' for k, v in attrs)
        return f"<{name}{joined}{tail}"
    # >1 attribute (xml namespaces count): tag name alone, then every attribute
    # on its own line at a 4-space continuation indent, closing bracket appended
    # to the last attribute.
    cont = pad + ATTR_INDENT
    lines = [f"<{name}"] + [f'{cont}{k}="{_esc_attr(v)}"' for k, v in attrs]
    return "\n".join(lines) + tail


def _render_children(node, level):
    """Render child nodes, preserving existing blank lines (whitespace-only
    text with >=2 newlines) but folding consecutive blank lines into one and
    stripping any whitespace from them."""
    parts = []
    pending_blank = False
    for c in node.childNodes:
        if c.nodeType == Node.TEXT_NODE and not c.data.strip():
            if c.data.count("\n") >= 2:
                pending_blank = True
            continue
        if pending_blank:
            parts.append("")  # one empty line (no whitespace)
            pending_blank = False
        parts.append(_serialize(c, level))
    if pending_blank and parts:
        parts.append("")
    return "\n".join(parts)


def _serialize(node, level):
    pad = INDENT * level
    t = node.nodeType
    if t == Node.ELEMENT_NODE:
        attrs = _attrs(node)
        kids = [c for c in node.childNodes if c.nodeType in _STRUCTURAL
                or (c.nodeType == Node.TEXT_NODE and c.data.strip())]
        if not kids:
            return pad + _start_tag(node.tagName, attrs, pad, True)
        # Leaf element with a single text child: keep text verbatim (never
        # strip -- leading/trailing whitespace may be significant).
        if len(kids) == 1 and kids[0].nodeType == Node.TEXT_NODE:
            st = _start_tag(node.tagName, attrs, pad, False)
            return f"{pad}{st}{_esc_text(kids[0].data)}</{node.tagName}>"
        st = _start_tag(node.tagName, attrs, pad, False)
        inner = _render_children(node, level + 1)
        return f"{pad}{st}\n{inner}\n{pad}</{node.tagName}>"
    if t == Node.TEXT_NODE:
        return pad + _esc_text(node.data.strip())
    if t == Node.COMMENT_NODE:
        return f"{pad}<!--{node.data}-->"
    if t == Node.CDATA_SECTION_NODE:
        return f"{pad}<![CDATA[{node.data}]]>"
    if t == Node.PROCESSING_INSTRUCTION_NODE:
        return f"{pad}<?{node.target} {node.data}?>"
    return ""


def format_xml(raw):
    """raw: bytes. Returns formatted str, or raises SkipFile when unsafe."""
    decl = _DECL_RE.match(raw)
    if decl:
        enc = _ENC_RE.search(decl.group(0))
        if enc and enc.group(1).lower() not in (b"utf-8", b"utf8"):
            raise SkipFile("non-utf-8 encoding")
    doc = minidom.parseString(raw)
    if doc.doctype is not None:
        raise SkipFile("DOCTYPE")
    if _is_unsafe(doc):
        raise SkipFile("mixed content / xml:space=preserve")
    parts = [decl.group(0).decode("utf-8")] if decl else []
    for n in doc.childNodes:
        s = _serialize(n, 0)
        if s.strip():
            parts.append(s)
    return "\n".join(parts) + "\n"


# --- driver -----------------------------------------------------------------
def process(rel, fix):
    """Returns one of: 'ok', 'changed', 'skip:<reason>', 'error:<reason>'."""
    path = REPO / rel
    raw = path.read_bytes()
    try:
        original = raw.decode("utf-8")
        formatted = format_json(original) if rel.endswith(".json") else format_xml(raw)
    except SkipFile as e:
        return f"skip:{e}"
    except Exception as e:  # parse / decode error
        return f"error:{e}"
    if formatted == original:
        return "ok"
    if fix:
        path.write_text(formatted, encoding="utf-8")
    return "changed"


def tracked_files():
    out = subprocess.run(
        ["git", "-C", str(REPO), "ls-files", "-z", "*.json", "*.xml"],
        capture_output=True, text=True, check=True).stdout
    return [p for p in out.split("\0") if p]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["all", "files"])
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--fix", action="store_true")
    ap.add_argument("--verbose", action="store_true", help="list skipped files")
    args = ap.parse_args()

    if args.mode == "all":
        rels = [r for r in tracked_files() if in_scope(r)]
    else:
        rels = [Path(p).resolve().relative_to(REPO).as_posix() for p in args.paths]
        rels = [r for r in rels if in_scope(r)]

    changed, errors, skipped = [], [], []
    for rel in rels:
        if not (REPO / rel).is_file():
            continue
        result = process(rel, args.fix)
        if result == "changed":
            changed.append(rel)
            print(("formatted " if args.fix else "needs formatting: ") + rel)
        elif result.startswith("error:"):
            errors.append((rel, result[6:]))
            print(f"ERROR {rel}: {result[6:]}", file=sys.stderr)
        elif result.startswith("skip:"):
            skipped.append((rel, result[5:]))
            if args.verbose:
                print(f"skipped {rel}: {result[5:]}")

    if args.verbose or skipped:
        print(f"({len(skipped)} file(s) skipped as unsafe to reflow)", file=sys.stderr)
    if errors:
        sys.exit(f"{len(errors)} file(s) failed to parse.")
    if changed and not args.fix:
        sys.exit("Run: tools/python/format_json_xml.py all --fix")


if __name__ == "__main__":
    main()
