# Release Management

## Apple App Store

### Upload metadata and screenshots to the App Store

Use [GitHub Actions](../.github/workflows/ios-release.yaml).

### Check metadata

Use [GitHub Actions](../.github/workflows/ios-check-metadata.yaml).

Local check:

```bash
./tools/python/check_store_metadata.py ios
```

### Downloading screenshots from the App Store

Get xcode/keys/appstore.json - App Store API Key.

Get screenshots/ - a repository with screenshots.

Download metadata:

```bash
cd xcode
./fastlane download_metadata
```

Download screenshots:

```bash
cd xcode
./fastlane download_screenshots
```

## Google Play

### Upload metadata and screenshots to Google Play

Use [GitHub Actions](../.github/workflows/android-release-metadata.yaml).

### Uploading a new version to Google Play

Use [GitHub Actions](../.github/workflows/android-release.yaml).

Promote version to "Production" manually in Google Play Console.

### Uploading a new version to Huawei AppGallery

Use [GitHub Actions](../.github/workflows/android-release.yaml).

### Checking metadata

Use [GitHub Actions](../.github/workflows/android-check-metadata.yaml).

Checking locally:

```bash
./tools/python/check_store_metadata.py android
```

Google release notes are per-locale symlinks to the shared F-Droid notes, filtered
using `tools/python/google_play_locales.json`. After adding or removing shared
release-note locales, regenerate and commit the Google links:

```bash
python3 tools/python/google_play_release_notes.py sync
```

To export validated notes for pasting into Play Console:

```bash
./tools/unix/update_gplay_relnotes.sh > /tmp/google-play-release-notes.txt
```

The export uses production notes when present, falling back to `default.txt`.
Pass a track name as the first argument to export notes for another track.

The `fa-IR` locale reuses the shared `fa` notes when no separate translation exists.
Google publishing tasks validate both the source and merged publishing metadata,
including track-specific notes. For an existing draft, replace its release notes
with the filtered export: updating or promoting through the publishing plugin can
retain old locale entries.

### Downloading metadata and screenshots from Google Play

Get `android/google-play.json` - Google Play API Key.

Get `screenshots/` - a repository with screenshots.

Download metadata:

```bash
./tools/android/download_googleplay_metadata.sh
```
