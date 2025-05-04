# Release Management

## Apple App Store

### Upload metadata and screenshots to the App Store

Use [Forgejo Actions](../.forgejo/workflows/ios-release.yaml).

### Check metadata

Use [Forgejo Actions](../.forgejo/workflows/ios-check.yaml).

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

Use [Forgejo Actions](../.forgejo/workflows/android-release-metadata.yaml).

### Uploading a new version to Google Play

Use [Forgejo Actions](../.forgejo/workflows/android-release.yaml).

Promote version to "Production" manually in Google Play Console.

### Uploading a new version to Huawei AppGallery

Use [Forgejo Actions](../.forgejo/workflows/android-release.yaml).

### Checking metadata

Use [Forgejo Actions](../.forgejo/workflows/android-check.yaml).

Checking locally:

```bash
./tools/python/check_store_metadata.py android
```

### Downloading metadata and screenshots from Google Play

Get `android/google-play.json` - Google Play API Key.

Get `screenshots/` - a repository with screenshots.

Download metadata:

```bash
./tools/android/download_googleplay_metadata.sh
```
