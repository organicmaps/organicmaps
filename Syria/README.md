# Syria Organic Maps

A branded fork of [Organic Maps](https://organicmaps.app) for Syria with:

- phone number + SMS code login (login is only needed for tracking; the map works without it);
- activity tracking: one GPS point every 5 s, works offline, pause/resume, live km and time, local history;
- background sync of sessions, points and activity events to the `syria-om-api` backend (LoopBack 4 + PostGIS);
- an activity log entry whenever the user downloads or updates map data (switchable in Settings);
- Syria map data downloaded automatically on the first launch from our own map server;
- Arabic UI by default, English available through the system per-app language setting.

Everything added by the fork lives in `android/app/src/main/java/app/organicmaps/syria/` plus small hooks in
`LocationHelper`, `MwmApplication`, `MwmActivity`, `SettingsPrefsFragment` and `DownloadResourcesLegacyActivity`,
so upstream Organic Maps changes stay easy to merge.

## Configuration

| What | Where |
|---|---|
| App id / name | `android/build.gradle` (`appId`), `android/settings.gradle`, `android/libs/branding/.../donottranslate.xml` |
| Icons | `android/libs/branding/src/main/res/` (generated from `Syria/assets/osm-syria-logo.svg`) |
| Tracking API URL | `-PsyriaApiUrl=http://host:3000` at build time (default `http://192.168.0.31:3000`, see `android/app/build.gradle`) |
| Map server | `private.h`: `DEFAULT_URLS_JSON` (default `http://192.168.0.31:8080/`), `METASERVER_URL` empty |
| Privacy policy | `SYRIA_PRIVACY_URL` in `android/app/build.gradle` |
| Release signing | `Syria/keystore/release.jks` + `android/app/secure.properties` (both ignored by git, see `Syria/keystore/README.md`) |
| Languages | `supportedLocalizations=ar,en` in `android/gradle.properties` |

## Build

```bash
cd android
./gradlew assembleGoogleDebug -Parm64                  # Pixel / any phone with Google services
./gradlew assembleGoogleRelease -Parm64                # signed with Syria/keystore/release.jks
./gradlew assembleHuaweiRelease -Parm64                # Huawei build (same code, AppGallery id/version code)
./gradlew :app:testGoogleDebugUnitTest --tests "app.organicmaps.syria.*"
```

APKs land in `android/app/build/outputs/apk/<flavor>/<type>/`; signed release builds are also copied to
`Syria/dist/` (gitignored). A release APK is ~41 MB per ABI (debug builds are unstripped, ~115 MB).

## Running the whole thing locally

1. Backend: `cd ../syria-om-api && cp .env.example .env && docker compose up -d --build` (API :3000, PostGIS :5433, maps :8080).
2. Maps: publish a map set with `maps-pipeline/` (see its README) so `maps-pipeline/out/maps/<version>/Syria.mwm` exists.
   The server publishes World + WorldCoasts (from the Organic Maps CDN) + Syria. `data/countries.json` in this repo
   lists exactly those three and its `v` must equal the version folder on the server (currently 261116);
   `data/packed_polygons.bin` is Syria-only (the pipeline README explains how both are regenerated when countries
   are added).
3. Phone on the same Wi-Fi: install the APK, first launch downloads World + Syria from `http://<PC IP>:8080/`.
4. Menu → "Activity tracking" → sign in with any Syrian number and code `123456` (fake SMS provider).
5. Admin dashboard: `http://<PC IP>:3000/admin/`.

## Test checklist (device on the same Wi-Fi as the PC)

1. Fresh install → the first screen downloads World + Syria automatically; the map, search and routing work without
   any login.
2. Menu → "Activity tracking" opens the login screen; enter a number (prefix +963 is prefilled) and code `123456`.
3. Start tracking, walk/drive (or play a GPX with a mock-location app), switch to airplane mode, pause, resume, stop. The
   notification and the tracking screen show elapsed time and km; the history list keeps the session.
4. Network back → "Sync now" (or wait ≤15 min). The session, segments, points and events appear in the admin dashboard with the
   route drawn on the map; the server distance is within a few percent of the client value.
5. Settings → Account: logout keeps unsynced data; log in again and sync.
6. Downloader → update Syria when a new version is published (`data_version.json`); with the setting on, a `map_update` event
   appears in the activity log.

## Not done yet / later

- Google sign-in (backend keeps an `auth_identities` table for it).
- Real SMS provider and the Syrian map mirror (one more URL in `DEFAULT_URLS_JSON`).
- Huawei devices without Google services use Android's own location provider; HMS Location Kit is optional.
