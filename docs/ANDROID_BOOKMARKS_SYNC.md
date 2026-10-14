# Android bookmarks sync with Google Drive

Design for two-way synchronization of bookmarks and tracks between Android devices signed in to the same
Google account (#7408). Status: proposal.

## Goals and non-goals

Goals:
- Keep bookmarks and tracks identical on all Android devices of one Google account, without losing edits.
- Opt-in, Google flavor only. No Organic Maps servers, no secrets in the repository.
- Store data in the same per-category KML files that iOS iCloud sync uses, so iOS can join the same storage later.

Non-goals for the first version:
- iOS support, Android ↔ iOS sync.
- Real-time push of remote changes, periodic background sync while the app is not running.
- Selective sync, version history UI, syncing app settings or `bm.json` (category sorting).

The Android system Auto Backup (`android/app/src/main/res/xml/backup_content*.xml`) stays as is. It restores data
when the app is installed, but it is per device and does not sync.

## Decisions

| Topic | Decision |
|---|---|
| Storage | Hidden `appDataFolder`, scope `https://www.googleapis.com/auth/drive.appdata` (non-sensitive) |
| Unit of sync | One flat `*.kml` file in the bookmarks directory, identified by its file name |
| Change detection | Snapshot reconciliation by content: local MD5, Drive `md5Checksum`, last synced state (base) |
| Conflicts | Edited on both sides → keep the local version as a copy named after the device model |
| Deletions | Soft on both sides: core `.Trash` locally, a `.Trash` folder inside `appDataFolder` in Drive |
| Flavors | Drive code in a module linked only by `googleImplementation`; no provider means no sync UI |

The reconciliation model follows the iOS rework in #13232 ("Reconcile iCloud files by content"), not the
mtime-based logic in `iphone/Maps/Core/iCloud/SynchronizationStateResolver.swift`.

## Architecture

```
libs/map (C++)            existing ReloadBookmark, GetCategoryByFileName, DeleteBmCategory(id, false)
   │ JNI (all flavors)
:sdk BookmarkManager      + category file JNI (see "Core and JNI")
   │
:sdk:sync:core            new, Kotlin, all flavors, no Google dependencies
   ├─ Reconciler          pure function (local, cloud, base) → actions
   ├─ LocalStore          lists bookmarks/*.kml with MD5, writes files on the core File thread
   ├─ BaseStateStore      files/sync/<provider>.json
   ├─ SyncEngine          one pass at a time, applies actions, persists base after each action
   ├─ CloudStore          interface: list, download, upload, moveToTrash, purge, account
   └─ CloudSyncRegistry   provider registry, same pattern as sdk/wear/core WearBridge
   │
:sdk:sync:gms             new, linked with googleImplementation only
   ├─ DriveAuthorizer     play-services-auth AuthorizationClient
   ├─ DriveCloudStore     Drive REST v3 over OkHttp + org.json
   └─ DriveSyncInitProvider  manifest ContentProvider that registers the provider,
                             same pattern as sdk/wear/gms WearBridgeInitProvider
   │
:app
   ├─ BookmarksSyncWork   CoroutineWorker, unique work, network constraint
   └─ BookmarksSyncSettingsFragment
```

`:sdk:sync:core` does not know about Google. Without `:sdk:sync:gms` (fdroid, web, huawei) the registry is empty,
so the settings item is hidden and nothing runs. Another provider (for example Nextcloud, #10199) can implement
`CloudStore` in its own module.

## Local state

- Bookmarks directory: `GetBookmarksDirectory()`, which is `getFilesDir()/bookmarks` on Android. Only regular
  `*.kml` files at the top level are synced. Hidden files and core temporary files (`*.kml.tmp*`) are ignored.
- Base state: `getFilesDir()/sync/google_drive.json`, written with temp file + rename:
  ```json
  {
    "version": 1,
    "account": "<Drive permissionId>",
    "trashFolderId": "<Drive id or null>",
    "files": { "Paris.kml": { "cloudId": "<Drive id>", "md5": "<hex>" } }
  }
  ```
- The base state is outside `bookmarks/`, so Auto Backup does not restore it. A device restored from Auto Backup
  starts with an initial reconciliation, and identical files are matched by MD5 without copies.
- Hash: lowercase hex MD5 of the file bytes, the same value Drive returns in `md5Checksum`. It detects changes only;
  it is not a security measure.

## Reconciliation

For every file name the pass knows L (local MD5), C (Drive MD5) and B (base MD5). A base entry applies only to the
Drive file it was recorded for: when another file owns the name now (this device's upload lost a duplicate race), the
name is reconciled as if it had no base entry.

Files with a base entry:

| L | C | Action |
|---|---|---|
| = B | = B | nothing |
| ≠ B | = B | upload L |
| = B | ≠ B | download C, replace local file, reload category |
| ≠ B | ≠ B, L = C | update base only |
| ≠ B | ≠ B, L ≠ C | conflict: local version becomes a copy, Drive version replaces the original name |
| missing | = B | deleted locally: move Drive file to `.Trash` |
| missing | ≠ B | deleted here, edited elsewhere: download C (an edit wins over a deletion) |
| = B | missing | deleted elsewhere: move local category to core `.Trash` |
| ≠ B | missing | deleted elsewhere, edited here: upload L |
| missing | missing | drop the base entry |

Files without a base entry (new files, initial sync, after a reset):

| L | C | Action |
|---|---|---|
| present | missing | upload L |
| missing | present | download C |
| present | present, L = C | record base |
| present | present, L ≠ C | conflict as above, except an empty local category (no bookmarks and tracks, for example the default "My Places" on a new device) is replaced by the Drive version without a copy |

Conflict copy:
- File name `<base> (<device>).kml`, made unique with the core rules (`GenerateUniqueFileName`). The device name is
  `Settings.Global "device_name"`, falling back to `Build.MODEL`, with characters invalid in file names removed.
- The category inside the copy is renamed to `<name> (<device>)`, so the lists are distinguishable in the UI.
- The copy is uploaded as a new file by the next pass.
- If the Drive version has no placemarks (an untouched default list of another device), the local version is uploaded
  over it instead, without a copy.

Safety rules:
1. Account change or vanished data: if the Drive `permissionId` differs from the base, Drive has no files while the
   base is not empty (the user deleted hidden app data in Drive settings), or another device is deleting all data (a
   `.reset` marker file younger than one hour), the base is cleared and the pass runs as an initial reconciliation.
   Nothing is deleted locally because of it.
2. Re-check before destructive actions: before a local file is replaced or trashed, its MD5 is recomputed on the
   core File thread; before a Drive file is overwritten or trashed, `files.get` returns its current MD5. If either
   changed since the snapshot, the action is skipped until the next pass.
3. No local apply while bookmark screens defer saving: when `BookmarkManager.areNotificationsEnabled()` is false
   (set by `BaseBookmarksActivity`), downloads, replacements and local trash are postponed, because a reload would
   drop unsaved in-memory edits. Uploads continue.
4. Duplicate names in Drive (two devices created `Paris.kml` before seeing each other): the file with the earliest
   `createdTime` (then the smallest id) owns the name. Other duplicates with the same content as the owner or an
   existing local file are moved to `.Trash`; the rest are downloaded under a unique local name and then moved to
   `.Trash`. All devices apply the same rule, so the state converges.
5. Order inside a pass: conflict copies, downloads, uploads, deletions. The base is persisted after every successful
   action, so an interrupted pass continues safely next time.
6. Drive `.Trash` files are deleted permanently 30 days after they were trashed (`appProperties.trashedAt`).

Known effects:
- Category visibility is stored in KML, so hiding a list is synced too.
- No-op rewrites by core (address updates after sorting by distance, saving unchanged data) do not change the MD5
  and cause no uploads.

## Google Drive

### Authorization

- `Identity.getAuthorizationClient(context).authorize(...)` with the `drive.appdata` scope. No Google Sign-In,
  no client id in the code, no backend: the Android OAuth client is matched by package name and signing SHA-1.
- First enable from the settings screen: if the result `hasResolution()`, the account picker and consent screen are
  launched with `IntentSenderRequest`. Google Play services cache and refresh the token afterwards.
- Background passes call `authorize()` again. If a resolution is required, the status becomes "Sign-in required";
  no UI is shown from the background.
- HTTP 401: invalidate the token with `GoogleAuthUtil.clearToken` and retry once.
- Account identity: `about.get?fields=user(permissionId,emailAddress)` (allowed with `drive.appdata`). `permissionId`
  is stored in the base, `emailAddress` is shown in settings.
- Disabling sync stops passes, deletes the local base first and then revokes access (`AuthorizationClient.revokeAccess`).
  Data in Drive stays until the user taps "Delete data from Google Drive", which also disables sync on this device,
  forgets the base and deletes the files between creating and removing the `.reset` marker.
- `play-services-auth` version: the newest one whose resolved `play-services-base`/`basement` still support
  minSdk 21 (21.1.x at the time of writing). Pin it in `libs.versions.toml` with the usual
  `# Newer versions require minSdk 23` comment.

### REST calls

| Operation | Request |
|---|---|
| list | `GET files?spaces=appDataFolder&q='appDataFolder' in parents&fields=nextPageToken,files(id,name,md5Checksum,size,createdTime)&pageSize=1000` |
| download | `GET files/{id}?alt=media` into a temporary file |
| create / update | resumable: `POST upload/drive/v3/files?uploadType=resumable` or `PATCH upload/drive/v3/files/{id}?uploadType=resumable`, then `PUT` to the session URI |
| trash | `PATCH files/{id}?addParents=<trashFolderId>&removeParents=appDataFolder` with `appProperties.trashedAt` |
| purge | `DELETE files/{id}` |

- Uploads are always resumable: multipart is limited to 5 MB and recorded tracks can be larger.
- Files inside `appDataFolder` cannot be trashed with the Drive trash, hence the `.Trash` folder, created on the first
  deletion.
- Errors: `403 storageQuotaExceeded` stops the pass with a "Google Drive is full" status; `429`,
  `403 rateLimitExceeded`/`userRateLimitExceeded`, `5xx` and network errors return `Result.retry()` (WorkManager
  exponential backoff); other `4xx` set an error status with the message.
- Quota: an idle pass costs one `files.list` (100 units), each change 50–200 units; the free project limit is
  400,000,000 units per day.

### Google Cloud setup (maintainers)

- A project with the Drive API enabled.
- OAuth consent screen with app name, logo, privacy policy URL and the `drive.appdata` scope (basic verification).
- Android OAuth clients:
  - `app.organicmaps` with the Play App Signing SHA-1;
  - `app.organicmaps.beta` with the beta signing SHA-1;
  - `app.organicmaps.debug` with the SHA-1 of the committed `android/app/debug.keystore`, shared by all developers.
- While the consent screen is in "Testing" mode, up to 100 test accounts can sign in, enough for development.
- A package + SHA-1 pair can be registered in one project only, so end-to-end testing depends on this project.

## Triggers and threading

A pass runs only when sync is enabled, a provider is registered and authorization succeeded:
1. App start, after `onBookmarksLoadingFinished`.
2. Local changes: `onBookmarksChanged` enqueues the unique work with a 5 s initial delay and `REPLACE`, so a burst
   of edits produces one pass. The native callback has a single slot, already bound in
   `android/sdk/src/main/cpp/app/organicmaps/sdk/bookmarks/data/BookmarkManager.cpp`, so the Java side fans it out.
3. App foreground and background (`ProcessLifecycleOwner`, `KEEP`): pull remote changes on return, push changes
   saved when bookmark screens close.
4. "Sync now" in settings.

`BookmarksSyncWork` follows `app/src/main/java/app/organicmaps/background/OsmUploadWork.kt`:
- unique work `BookmarksSync`; a background pass can't ask about mobile data, so it needs `UNMETERED` unless the
  mobile data setting is "Always" (`CONNECTED`, or `NOT_ROAMING` when roaming is not accepted);
- if WorkManager starts the process without an initialized core, the work returns `success()` and does nothing: the
  next app start runs a pass, and the snapshot model loses nothing;
- `SyncEngine` holds a `Mutex`, so passes never overlap.

| Work | Thread |
|---|---|
| network, hashing for the snapshot, base JSON | `Dispatchers.IO` |
| `ReloadBookmark`, category lookup by file, move to core `.Trash`, `areNotificationsEnabled`, `isEmpty`, rename | `Dispatchers.Main` (core checks the thread in release too) |
| re-check MD5 and write or rename files in `bookmarks/` | core `Platform::Thread::File`, via `runOnFileThread` |

Core saves categories asynchronously on its File thread (`SaveBookmarks` → `WriteToTempAndRenameToFile`). Replacing a
file from another thread could let a queued save write old data over the downloaded version, and the next pass would
upload it back, reverting the remote edit. Running the re-check and rename on the same serial File thread orders them
after all queued saves. `ReloadBookmark` is then called on Main; it does not write the file back
(`CreateCategories` uses `NotifyChanges(false)`), so there is no echo.

Conflict copy sequence: copy the local bytes to the new name on the File thread, reload the copy and rename its
category on Main, then replace the original file with the Drive version after an MD5 re-check.

Remaining race: an edit of the same category in the few milliseconds between the rename and `ReloadBookmark` wins
over the Drive version. No data is lost locally; the remote edit is overwritten by the next upload.

## Core and JNI

New JNI in `android/sdk/src/main/cpp/app/organicmaps/sdk/bookmarks/data/BookmarkManager.cpp` and
`BookmarkManager.java`, reusing existing core functions:

| Java | Core |
|---|---|
| `nativeGetBookmarksDir()` | `GetBookmarksDirectory()` (the existing `Framework.nativeGetBookmarkDir` returns the settings dir) |
| `nativeGetCategoryIdByFile(path)` | `GetCategoryByFileName(path)`, `-1` if none |
| `nativeReloadCategoryFile(path)` | `ReloadBookmark(path)` |
| `nativeMoveCategoryToTrash(id)` | `DeleteBmCategory(id, false)`; the existing `nativeDeleteCategory` deletes permanently |
| `nativeRunOnFileThread(Runnable)` | `GetPlatform().RunTask(Platform::Thread::File, ...)`; the runnable must be short and must not call `BookmarkManager` |

Existing category APIs (`BookmarkCategory` isEmpty, `nativeSetName`) cover the empty check and the copy rename.

## Settings UI and errors

- Privacy category: a "Google Drive sync (Beta)" item next to "Bookmarks backup", visible only when a provider is
  registered. It opens `BookmarksSyncSettingsFragment`:
  - "Sync bookmarks and tracks" switch;
  - account email;
  - status: "Synced at 14:05", "Syncing…", "Sign-in required" (tap to authorize), "Google Drive is full", or an
    error message;
  - "Sync now";
  - "Delete data from Google Drive", with confirmation.
- First enable, as on iOS: a "Beta, make a backup first" dialog with Cancel, Export (the existing export of all
  bookmarks to KMZ) and Enable. Then authorization and the initial pass.
- `SyncEngine` exposes `StateFlow<SyncStatus>`: `Idle(lastSync)`, `Running`, `NeedsSignIn`, `Error(kind)`. The last
  sync time is persisted. Transient network errors are not shown while WorkManager retries.
- All strings go to `data/strings/strings.txt`.

## Testing

- `:sdk:sync:core` JVM unit tests:
  - table tests of `Reconciler` for every row above and every safety rule (vanished Drive data, account change,
    duplicate names, empty local category);
  - test cases ported from `iphone/Maps/Tests/Core/iCloudTests/` and #13232;
  - `SyncEngine` with a fake `CloudStore`, a temporary directory and a fake core port: action order, base persisted
    after each action, resuming an interrupted pass, skipping an action when the MD5 changed.
- `:sdk:sync:gms` tests with OkHttp `MockWebServer`: request shapes, pagination, resumable uploads, error mapping
  (401 retry, 403 quota, 429). `org.json:json` as a test dependency, because `android.jar` stubs are not usable in
  JVM tests.
- Core: a `libs/map/map_tests` test for moving a category to `.Trash` by file path.
- CI (`.github/workflows/android-check.yaml`): PR checks do not build the Google flavor, so add
  `:sdk:sync:core:testDebugUnitTest`, `:sdk:sync:gms:testDebugUnitTest` and `:sdk:sync:gms:assembleDebug`.
  Kotlin modules get ktlint and detekt through `enableKotlin`.
- Manual, two devices with one test account: create, edit, conflict, delete, offline edits, account switch, deleting
  hidden app data in Drive settings.

## Implementation plan

One pull request, stacked on top of the "Bookmarks backup" settings item. Each commit builds and passes tests:

| # | Commit | Content |
|---|---|---|
| 1 | `[docs] Add Android bookmarks sync design` | this document |
| 2 | `[android][bookmarks] Expose category file operations to JNI` | JNI above, core test |
| 3 | `[android] Add bookmarks sync core module` | `Reconciler`, `SyncEngine`, stores, registry, tests; not wired into the app |
| 4 | `[android] Add Google Drive sync provider` | authorization, REST client, Google flavor wiring, version pin, CI |
| 5 | `[android] Add bookmarks sync settings and scheduling` | worker, triggers, settings screen, strings |
| 6 | `[strings] Regenerated` | generated Android string resources |

Commits 2–4 are developed and tested without Google (fakes, `MockWebServer`). Commit 5 needs the Google Cloud project
for end-to-end testing before the pull request is merged.

## Risks and open questions

- Core `.Trash` on Android has no UI ("Recently deleted" exists on iOS only), so categories trashed by sync can be
  recovered only manually for now. A recovery UI can follow separately.
- The `play-services-auth` version is limited by minSdk 21; newer releases need minSdk 23.
- iOS joining later must use the same Google Cloud project to share `appDataFolder`, and decide how Drive sync
  coexists with iCloud sync.
