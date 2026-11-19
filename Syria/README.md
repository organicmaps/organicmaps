# Syria Organic Maps

A branded fork of [Organic Maps](https://organicmaps.app) for Syria with:

- sign-in (only needed for recording; the map works without it) with phone number + a 6-digit PIN the user
  chooses (a new number registers right away, marked "unverified" until confirmed by Telegram or SMS, which
  then takes the account over), the Telegram bot (also the way back after a forgotten PIN), or anonymously
  ("Record anonymously": no phone number, recordings and live location are kept under the device ID shown in
  Settings, proven by a secret only the app holds; `POST /auth/device`); the PIN fields
  work with Android autofill / password managers; Syria (+963) is the default country code. SMS codes stay
  switched off on the server until a real SMS provider is set (`ALLOW_FAKE_OTP=true` re-enables the fixed
  test code for local development only). The sign-in screen opens with spoken instructions (? icon);
- activity recording: one GPS point every 5 s, works offline, pause/resume, live km and time, local history
  (10 per page, durations in days/hours/minutes/seconds);
- a traffic-light recording button above the zoom buttons (green: recording; red, gently pulsing to invite a
  recording: not recording or paused) that opens the recording screen, and an internet indicator under the layers
  button. The recording screen background follows the same light; sync colours (green synced, orange
  pending/syncing with a progress bar, red failed/signed out) are used only in its sync section;
- search logging: every search (query, picked result, GPS position) is queued offline as a `search` event and
  uploaded under the account once signed in (backend view `search_logs`);
- background sync of sessions, points and activity events to the `syria-om-api` backend (LoopBack 4 + PostGIS);
- an activity log entry whenever the user downloads or updates map data (switchable in Settings);
- new map data without an app update: on start the app reads `data_version.json` from the map server and offers
  "Update all" when a newer version is published; the downloader shows when each map was last updated on the phone;
- Syria map data downloaded automatically on the first launch from our own map server;
- Arabic UI by default; English through Settings → "App language" or the system per-app language setting (the
  app offers a restart afterwards so the map, search and voice switch too). "Map language" only changes map
  labels, and only where OpenStreetMap has a name in that language;
- route optimization, larger map labels ("Increase font size on the map") and "Keep screen on" (while the map
  is shown) on by default; About screen with the fork / OSM Syria notice.

The Arabic website and privacy policy are in `Syria/website/` (`index.html`, `privacy.html`, self-contained)
for upload to openstreetmap.sy.

Everything added by the fork lives in `android/app/src/main/java/app/organicmaps/syria/` plus small hooks in
`LocationHelper`, `MwmApplication`, `MwmActivity`, `SettingsPrefsFragment`, `DownloadResourcesLegacyActivity`,
`MapButtonsController`, `SearchFragment`, `HelpFragment`, `DownloaderAdapter` and the map buttons / about / downloader layouts,
so upstream Organic Maps changes stay easy to merge.

## Configuration

| What | Where |
|---|---|
| App id / name | `android/build.gradle` (`appId`), `android/settings.gradle`, `android/libs/branding/.../donottranslate.xml` |
| Icons | `android/libs/branding/src/main/res/` (generated from `Syria/assets/osm-syria-logo.svg`) |
| API + map server | Public builds (Google Play, APK): always `https://organicmaps.openstreetmap.sy` (`SYRIA_API_URL` / `SYRIA_MAPS_URL` in `android/app/build.gradle`; `DEFAULT_URLS_JSON` in `private.h` for the other platforms) |
| Development server | Debug builds, and release builds made with `-PsyriaDevSettings`, show Settings → "Data server (development)": production or the local Docker server (`http://192.168.0.31:3000` API, `:8080` maps; change the host with `-PsyriaLocalHost=<LAN IP>`). Switching signs out and restarts the app |
| Privacy policy / website | `SYRIA_PRIVACY_URL`, `SYRIA_WEBSITE_URL` in `android/app/build.gradle` |
| Release signing | `Syria/keystore/release.jks` + `android/app/secure.properties` (both ignored by git, see `Syria/keystore/README.md`) |
| Languages | `supportedLocalizations=ar,en` in `android/gradle.properties` |

## Build

```bash
cd android
./gradlew assembleGoogleDebug -Parm64                  # Pixel / any phone with Google services
./gradlew assembleGoogleRelease -Parm64                # signed with Syria/keystore/release.jks (production server only)
./gradlew assembleGoogleRelease -Parm64 -PsyriaDevSettings   # same, plus the server choice in Settings (test APKs)
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
4. Recording button above zoom (or Menu → "Activity recording") → sign in with any number and a new 6-digit PIN
   (the account is created on the spot), or with the Telegram bot.
5. Admin dashboard: `http://<PC IP>:3000/admin/`.

## Test checklist (device on the same Wi-Fi as the PC)

1. Fresh install → the first screen downloads World + Syria automatically; the map, search and routing work without
   any login.
2. The traffic-light button above zoom is red; it opens the login screen. Keep the country code `963` (or change it),
   enter the number (a leading 0 is fine) and a new 6-digit PIN: the account is created and Android offers to save the
   PIN. Settings → Account shows `00963 9xx xxx xxx` right away. Sign out and back in with autofill; a wrong PIN 5 times
   shows the lockout message; Telegram sign-in on a number without a PIN asks to set one; Settings → "Change PIN".
3. Start tracking, walk/drive (or play a GPX with a mock-location app), switch to airplane mode, pause, resume, stop. The
   notification and the tracking screen show elapsed time and km; the history list keeps the session.
4. Network back → "Sync now" (or wait ≤15 min). The session, segments, points and events appear in the admin dashboard with the
   route drawn on the map; the server distance is within a few percent of the client value.
5. Settings → Account: logout keeps unsynced data; log in again and sync.
6. Search offline while signed out, pick a result, then go online and sign in: the search appears in
   `SELECT * FROM search_logs` on the backend.
7. Downloader → update Syria when a new version is published (`data_version.json`); with the setting on, a `map_update` event
   appears in the activity log.

## Not done yet / later

- Google sign-in (backend keeps an `auth_identities` table for it).
- Real SMS provider and the Syrian map mirror (one more URL in `DEFAULT_URLS_JSON`).
- Huawei devices without Google services use Android's own location provider; HMS Location Kit is optional.
