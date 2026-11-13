#!/usr/bin/env python3
"""Maintain Google-only links to shared release notes and export Console text."""

import contextlib
import os
from pathlib import Path
import sys

from check_store_metadata import GPLAY_LOCALES, check_google


def sync_notes(repo_root):
    shared = repo_root / 'android/app/src/fdroid/play/release-notes'
    google = repo_root / 'android/app/src/google/play/release-notes'
    targets = {p.name: p for p in shared.iterdir() if p.name in GPLAY_LOCALES}
    if 'fa-IR' not in targets and 'fa' in targets:
        targets['fa-IR'] = targets['fa']
    if 'en-US' not in targets:
        raise ValueError('Shared release notes must include en-US')
    for target in targets.values():
        if not (target / 'default.txt').is_file():
            raise ValueError(f'Missing release notes: {target}/default.txt')

    # Unlink the old blanket symlink itself, never its shared contents.
    if google.is_symlink():
        google.unlink()
    google.mkdir(parents=True, exist_ok=True)
    entries = list(google.iterdir())
    for entry in entries:
        if not entry.is_symlink():
            raise ValueError(f'Refusing to replace non-symlink: {entry}')
    for entry in entries:
        if entry.name not in targets:
            entry.unlink()
    for locale, target in sorted(targets.items()):
        link = google / locale
        relative = os.path.relpath(target, google)
        if link.is_symlink():
            if os.readlink(link) == relative:
                continue
            link.unlink()
        link.symlink_to(relative, target_is_directory=True)


def export_notes(repo_root, track='production'):
    play_dir = repo_root / 'android/app/src/google/play'
    with contextlib.redirect_stdout(sys.stderr):
        if not check_google(play_dir):
            raise ValueError('Google Play metadata validation failed')
    blocks = []
    for locale in sorted((play_dir / 'release-notes').iterdir()):
        if locale.name not in GPLAY_LOCALES:
            continue
        # Match Gradle Play Publisher's track-specific precedence.
        for name in (track.replace(':', '-'), track.split(':')[-1], 'default'):
            note = locale / f'{name}.txt'
            if note.is_file():
                blocks.append(f'<{locale.name}>\n{note.read_text(encoding="utf-8").rstrip()}\n</{locale.name}>\n')
                break
    return ''.join(blocks)


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    try:
        match sys.argv[1:]:
            case ['sync']:
                sync_notes(root)
            case ['export']:
                print(export_notes(root), end='')
            case ['export', track]:
                print(export_notes(root, track), end='')
            case _:
                sys.exit(f'Usage: {sys.argv[0]} sync|export [TRACK]')
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        sys.exit(2)
