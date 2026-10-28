#!/usr/bin/env python3
"""Check local API documentation links and normalize AGP's generated navigation."""

import argparse
from dataclasses import dataclass
from html.parser import HTMLParser
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit


@dataclass
class Link:
    href: str
    start: int
    tag: str


class Page(HTMLParser):
    def __init__(self, text):
        super().__init__()
        self.text = text
        self.ids = set()
        self.links = []
        self.offsets = [0]
        # Match HTMLParser's line counting; other Unicode separators are ordinary text.
        for line in text.split("\n"):
            self.offsets.append(self.offsets[-1] + len(line) + 1)
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        if tag == "a" and "name" in attrs:
            self.ids.add(attrs["name"])
        if "href" in attrs:
            line, column = self.getpos()
            self.links.append(Link(attrs["href"], self.offsets[line - 1] + column, self.get_starttag_text()))


def local_target(root, page, href):
    if re.search(r"[\s<>]|%(?![0-9a-fA-F]{2})", href):
        raise ValueError(f"Invalid API documentation link in {page.relative_to(root)}: {href}")
    try:
        target = urlsplit(href)
    except ValueError as error:
        raise ValueError(f"Invalid API documentation link in {page.relative_to(root)}: {href}") from error
    if target.scheme or target.netloc or (target.path and not target.path.endswith(".html")):
        return None
    path = unquote(target.path)
    if path.startswith("/"):
        destination = root / path.lstrip("/")
    else:
        destination = page.parent / path if path else page
    return destination.resolve(), unquote(target.fragment)


def check_docs(directory, normalize=False):
    root = Path(directory).resolve()
    pages = {path: Page(path.read_text(encoding="utf-8")) for path in sorted(root.rglob("*.html"))}
    if normalize:
        # AGP 9.4.1 / Dokka 2.2.0 emits hierarchy pages and inherited-member
        # anchors without their definitions. Keep entries without details as
        # text in both the site and publication jars. Missing files other than
        # these three navigation pages must still fail validation.
        missing_navigation = {"package-tree.html", "overview-tree.html", "package-summary.html"}
        href_attribute = re.compile(r"""\s+href\s*=\s*(?:"[^"]*"|'[^']*'|[^\s>]+)""", re.IGNORECASE)
        for path, page in pages.items():
            text = page.text
            for link in reversed(page.links):
                target = local_target(root, path, link.href)
                if target is None:
                    continue
                destination, fragment = target
                missing_member = destination in pages and fragment and fragment not in pages[destination].ids
                missing_page = (
                    destination not in pages
                    and destination.parent == path.parent
                    and destination.name in missing_navigation
                )
                if missing_member or missing_page:
                    tag = href_attribute.sub("", link.tag, count=1)
                    text = text[: link.start] + tag + text[link.start + len(link.tag) :]
            if "overview_description" not in page.ids:
                text = text.replace("<p>See: <a>Description</a></p>", "")
            if text != page.text:
                path.write_text(text, encoding="utf-8")
        return check_docs(root)

    for path, page in pages.items():
        for link in page.links:
            target = local_target(root, path, link.href)
            if target is None:
                continue
            destination, fragment = target
            if destination not in pages or (fragment and fragment not in pages[destination].ids):
                raise ValueError(f"Broken API documentation link in {path.relative_to(root)}: {link.href}")
    return len(pages)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--normalize", action="store_true", help="Normalize generated AGP navigation before checking")
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    try:
        count = check_docs(args.directory, args.normalize)
    except ValueError as error:
        parser.exit(1, f"{error}\n")
    if not args.normalize:
        print(f"Checked {count} API documentation pages")


if __name__ == "__main__":
    main()
