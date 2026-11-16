#!/usr/bin/env python3

import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_store_metadata
from google_play_release_notes import export_notes, sync_notes


class GooglePlayReleaseNotesTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root = Path(self.temp_dir.name).resolve()
        self.shared = self.root / 'android/app/src/fdroid/play/release-notes'
        self.google = self.root / 'android/app/src/google/play'
        self.notes = self.google / 'release-notes'
        self.write_shared('en-US', 'English release notes\n')

    def write_shared(self, locale, text):
        note = self.shared / locale / 'default.txt'
        note.parent.mkdir(parents=True, exist_ok=True)
        note.write_text(text, encoding='utf-8')
        return note

    def check_google(self, generated=False):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return check_store_metadata.check_google(self.google, generated=generated)

    def test_migrate_blanket_link_preserves_all_shared_notes(self):
        excluded = ('as', 'bo', 'bs', 'jv', 'mai', 'mi', 'or', 'ug', 'uz')
        for locale in (*excluded, 'fa', 'fr-FR'):
            self.write_shared(locale, f'{locale} translation\n')
        track_note = self.shared / 'fr-FR/production.txt'
        track_note.write_text('Production-only translation\n', encoding='utf-8')
        before = {p.relative_to(self.shared): p.read_bytes() for p in self.shared.rglob('*.txt')}
        self.google.mkdir(parents=True)
        self.notes.symlink_to(os.path.relpath(self.shared, self.google), target_is_directory=True)

        sync_notes(self.root)

        self.assertFalse(self.notes.is_symlink())
        self.assertEqual({p.name for p in self.notes.iterdir()}, {'en-US', 'fa', 'fa-IR', 'fr-FR'})
        self.assertTrue(all(p.is_symlink() for p in self.notes.iterdir()))
        self.assertEqual((self.notes / 'fr-FR/production.txt').read_text(), track_note.read_text())
        self.assertEqual((self.notes / 'fa-IR/default.txt').read_bytes(), before[Path('fa/default.txt')])
        self.assertEqual({p.relative_to(self.shared): p.read_bytes() for p in self.shared.rglob('*.txt')}, before)
        self.assertTrue(self.check_google())

    def test_sync_removes_obsolete_google_link(self):
        source = self.write_shared('de-DE', 'German release notes\n')
        sync_notes(self.root)
        self.assertTrue((self.notes / 'de-DE').is_symlink())
        source.unlink()
        source.parent.rmdir()

        sync_notes(self.root)

        self.assertFalse(os.path.lexists(self.notes / 'de-DE'))
        self.assertEqual((self.shared / 'en-US/default.txt').read_text(), 'English release notes\n')
        self.assertTrue(self.check_google())

    def test_sync_preserves_unexpected_real_directory(self):
        unexpected = self.notes / 'de-DE'
        unexpected.mkdir(parents=True)
        sentinel = unexpected / 'default.txt'
        sentinel.write_text('Uncommitted translation\n', encoding='utf-8')

        with self.assertRaises(ValueError):
            sync_notes(self.root)

        self.assertEqual(sentinel.read_text(), 'Uncommitted translation\n')

    def test_huawei_locale_is_rejected_only_in_google_metadata(self):
        self.write_shared('as', 'Assamese release notes\n')
        google_note = self.notes / 'as/default.txt'
        google_note.parent.mkdir(parents=True)
        google_note.write_text('Assamese release notes\n', encoding='utf-8')
        for flavor in ('fdroid', 'google'):
            title = self.root / f'android/app/src/{flavor}/play/listings/as/title.txt'
            title.parent.mkdir(parents=True, exist_ok=True)
            title.write_text('Organic Maps\n', encoding='utf-8')
        with contextlib.chdir(self.root), contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            for kind, filename in (('release-notes', 'default.txt'), ('listings', 'title.txt')):
                self.assertTrue(check_store_metadata.check_path(f'android/app/src/fdroid/play/{kind}/as/{filename}'))
                self.assertFalse(check_store_metadata.check_path(f'android/app/src/google/play/{kind}/as/{filename}'))

    def test_broken_locale_link_fails_validation_and_export(self):
        sync_notes(self.root)
        (self.notes / 'fr-FR').symlink_to('missing', target_is_directory=True)

        self.assertFalse(self.check_google())
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()), self.assertRaises(ValueError):
            export_notes(self.root)

    def test_bulk_validation_rejects_huawei_only_listing(self):
        sync_notes(self.root)
        listing = self.google / 'listings/as/title.txt'
        listing.parent.mkdir(parents=True)
        listing.write_text('Organic Maps\n', encoding='utf-8')

        self.assertFalse(self.check_google())

    def test_track_specific_notes_obey_character_limit(self):
        sync_notes(self.root)
        track_note = self.shared / 'en-US/production.txt'
        track_note.write_text('é' * 500, encoding='utf-8')
        self.assertTrue(self.check_google())
        track_note.write_text('é' * 501, encoding='utf-8')

        self.assertFalse(self.check_google())
        with contextlib.chdir(self.root), contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertFalse(check_store_metadata.check_path('android/app/src/google/play/release-notes/en-US/production.txt'))

    def test_export_is_sorted_and_contains_only_google_locales(self):
        self.write_shared('fr-FR', 'Notes françaises\n')
        self.write_shared('as', 'Assamese release notes\n')
        sync_notes(self.root)

        with contextlib.redirect_stderr(io.StringIO()):
            result = export_notes(self.root)

        self.assertEqual(result, '<en-US>\nEnglish release notes\n</en-US>\n<fr-FR>\nNotes françaises\n</fr-FR>\n')

    def test_export_does_not_print_partial_notes_on_failure(self):
        self.write_shared('fr-FR', 'x' * 501)
        sync_notes(self.root)
        stdout = io.StringIO()

        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(io.StringIO()), self.assertRaises(ValueError):
            export_notes(self.root)

        self.assertEqual(stdout.getvalue(), '')

    def test_export_prefers_production_notes_over_default(self):
        sync_notes(self.root)
        (self.shared / 'en-US/production.txt').write_text('Production notes', encoding='utf-8')

        with contextlib.redirect_stderr(io.StringIO()):
            result = export_notes(self.root)

        self.assertEqual(result, '<en-US>\nProduction notes\n</en-US>\n')

    def test_export_uses_requested_track(self):
        sync_notes(self.root)
        (self.shared / 'en-US/production.txt').write_text('Production notes', encoding='utf-8')
        (self.shared / 'en-US/beta.txt').write_text('Beta notes', encoding='utf-8')

        with contextlib.redirect_stderr(io.StringIO()):
            result = export_notes(self.root, track='beta')

        self.assertEqual(result, '<en-US>\nBeta notes\n</en-US>\n')

    def test_form_factor_track_falls_back_to_track_then_default(self):
        sync_notes(self.root)
        production = self.shared / 'en-US/production.txt'
        production.write_text('Production notes', encoding='utf-8')
        wear = self.shared / 'en-US/wear-production.txt'
        wear.write_text('Wear notes', encoding='utf-8')

        for expected, remove in (
            ('Wear notes', wear),
            ('Production notes', production),
            ('English release notes', None),
        ):
            with self.subTest(expected=expected), contextlib.redirect_stderr(io.StringIO()):
                result = export_notes(self.root, track='wear:production')
                self.assertEqual(result, f'<en-US>\n{expected}\n</en-US>\n')
            if remove is not None:
                remove.unlink()

    def test_hidden_locale_directory_is_not_exported(self):
        sync_notes(self.root)
        backup = self.notes / '.backup'
        backup.mkdir()
        (backup / 'default.txt').write_text('Hidden backup', encoding='utf-8')
        (self.shared / 'en-US/.backup.txt').write_text('x' * 501, encoding='utf-8')

        with contextlib.redirect_stderr(io.StringIO()):
            result = export_notes(self.root)

        self.assertNotIn('.backup', result)
        self.assertNotIn('Hidden backup', result)
        self.assertIn('<en-US>', result)

    def test_generated_validation_ignores_only_empty_unsupported_locale(self):
        sync_notes(self.root)
        stale = self.notes / 'as'
        stale.mkdir()
        self.assertFalse(self.check_google())
        self.assertTrue(self.check_google(generated=True))

        for filename in ('default.txt', 'production.txt'):
            with self.subTest(filename=filename):
                note = stale / filename
                note.write_text('Stale unsupported notes', encoding='utf-8')
                self.assertFalse(self.check_google(generated=True))
                note.unlink()


if __name__ == '__main__':
    unittest.main()
