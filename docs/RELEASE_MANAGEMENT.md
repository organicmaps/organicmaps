# Release Management

Releases are built and uploaded by hand: there are no release workflows.

## Google Play

See [the publishing guide](nogps/google-play.md) (in Russian).

## Checking store metadata

GitHub Actions check the metadata in pull requests that change it:
[Android](../.github/workflows/android-check-metadata.yaml), [iOS](../.github/workflows/ios-check-metadata.yaml).

Local check:

```bash
./tools/python/check_store_metadata.py android
./tools/python/check_store_metadata.py ios
```
