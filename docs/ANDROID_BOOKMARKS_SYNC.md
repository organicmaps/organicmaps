# Android bookmarks sync with Google Drive

Design for two-way synchronization of bookmarks and tracks between Android devices signed in to the same
Google account (#7408).

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
   ├─ CloudStore          interface: list, createGeneration, currentMd5, download, upload, moveToTrash, purgeTrash,
   │                      deleteAll
   └─ CloudSyncRegistry   provider registry like sdk/wear/core WearBridge; creates the provider on first use
   │
:sdk:sync:gms             new, linked with googleImplementation only
   ├─ DriveAuthorizer     play-services-auth AuthorizationClient
   ├─ DriveCloudStore     Drive REST v3 over OkHttp + org.json
   └─ DriveSyncInitProvider  manifest ContentProvider that registers the provider factory,
                             like sdk/wear/gms WearBridgeInitProvider; Play services are
                             checked only when sync needs the provider
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
  `*.kml` files at the top level are synced, names with a leading dot included: core names the file after the list.
  Core temporary files (`*.kml.tmp*`) are ignored.
- Base state: `getFilesDir()/sync/google_drive.json`, written with temp file + rename:
  ```json
  {
    "version": 1,
    "generation": "<Drive id of .generation>",
    "trashFolderId": "<Drive id or null>",
    "lastSync": <milliseconds or null>,
    "files": { "Paris.kml": { "cloudId": "<Drive id>", "md5": "<hex>" } }
  }
  ```
- The base state is outside `bookmarks/`, so Auto Backup does not restore it. A device restored from Auto Backup
  starts with an initial reconciliation, and identical files are matched by MD5 without copies.
- Hash: lowercase hex MD5 of the file bytes, the same value Drive returns in `md5Checksum`. It detects changes only;
  it is not a security measure.

## Reconciliation

For every file name the pass knows L (local MD5), C (Drive MD5) and B (base MD5). A base entry applies only to the
Drive file it was recorded for. When another file owns the name now and the synced one is still listed, this device's
upload lost a duplicate race, and the name is reconciled as if it had no base entry. When the synced one is not listed,
it was deleted elsewhere and a new file took its name: an unchanged local file (L = B) is replaced by the new one once
`files.get` confirms the deletion (rule 2), otherwise the name is reconciled as if it had no base entry.

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
| = B | missing | deleted elsewhere: move local category to core `.Trash` once Drive confirms it (rule 2) |
| ≠ B | missing | deleted elsewhere, edited here: upload L (over the synced Drive file if it still exists, rule 2) |
| missing | missing | drop the base entry |

Files without a base entry (new files, initial sync, after a reset):

| L | C | Action |
|---|---|---|
| present | missing | upload L |
| missing | present | download C |
| present | present, L = C | record base |
| present | present, L ≠ C | conflict as above, except that a version without bookmarks and tracks (for example the untouched default "My Places" of a new device) yields to the other one without a copy |

Conflict copy:
- File name `<base> (<device>).kml`, built with the core rules (`GenerateValidAndUniqueFilePath`): invalid and control
  characters removed, at most 200 bytes, unique among the local files and the cloud files of the pass. The device
  name is `Settings.Global "device_name"`, falling back to `Build.MODEL`.
- The category inside the copy is renamed to `<name> (<device>)`, so the lists are distinguishable in the UI.
- The copy is uploaded as a new file by the next pass.

Safety rules:
1. Cloud generation: `appDataFolder` holds an empty `.generation` file, and the base records its Drive id. Another
   account has another generation; deleting all data from the app or hidden app data in Drive settings deletes it,
   and the next pass creates a new one (when two devices create it at once, the earliest `createdTime`, then the
   smallest id, wins). Only files created since the generation are listed: older ones are left from deleted data,
   which another device may still be deleting one by one, so lists are uploaded again rather than matched with them.
   If the generation differs from the base, the base is cleared and the pass runs as an initial reconciliation, so
   nothing is deleted locally because of it, even when another device has uploaded its lists since. Within one
   generation, a file missing in Drive was deleted on purpose.
2. Re-check before destructive actions: before a local file is replaced or trashed, its MD5 is recomputed on the
   core File thread; before a Drive file is overwritten or trashed, `files.get` returns its current MD5. If either
   changed since the snapshot, the action is skipped until the next pass. Duplicates (rule 4) are the exception:
   they are trashed without a re-check, and the device that keeps writing one sees that it lost the name.
   Listings can lag behind, while `files.get` by id can't: a file missing from the listing trashes the local
   category only once `files.get` reports its Drive file gone or in `.Trash`, and a local edit of such a file
   overwrites that Drive file while it is still unchanged. A file deleted between the listing and its download is
   skipped.
3. No local apply while it would lose edits or break a screen: while a bookmarks screen (`BaseBookmarksActivity`) is
   open, core may keep edits unsaved (`BookmarkManager.areNotificationsEnabled()` is false), and the screen, its
   dialogs and search results hold ids of bookmarks and tracks; while the Place Page shows a bookmark or track, it
   and the edit dialog opened from it hold their ids too. A reload or trash of their category replaces those ids.
   Downloads, replacements and local trash are postponed then, also when this starts while one of them waits for the
   network. Uploads continue, the last sync time stays as it was, and the pass runs again once the Place Page or the
   bookmarks screen is closed, or after the WorkManager retry delay while it stays open.
4. Duplicate names in Drive (two devices created `Paris.kml` before seeing each other): the file with the earliest
   `createdTime` (then the smallest id) owns the name. Other duplicates with the same content as the owner or an
   existing local file are moved to `.Trash`; the rest are downloaded under a unique local name and then moved to
   `.Trash`. A duplicate that the base still refers to (this device's upload lost the name) waits in Drive until the
   conflict copy of the name keeps the local version. All devices apply the same rule, so the state converges.
5. Order inside a pass: conflict copies, duplicates, downloads, uploads, deletions. The base is persisted after every
   successful action, so an interrupted pass continues safely next time.
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
- Background passes call `authorize()` again. If a resolution is required, the status becomes "Sign-in required", and
  other failures show as an error; no UI is shown from the background.
- HTTP 401: invalidate the token with `AuthorizationClient.clearToken` and retry once.
- The synced account is stored by the provider and passed to every `authorize()` (`setAccount`), so background passes
  never pick another one. Until it is known, the first authorized pass or the settings screen learn it from
  `about.get?fields=user(emailAddress)` (allowed with `drive.appdata`).
- Switching the account: `AccountPicker` with the device's Google accounts, then `authorize()` for the picked one. It
  becomes the synced account only once authorized; cancelling at any step keeps the current account. The settings
  screen holds the picked account while the consent screen is open, also across its recreation, so a background pass
  authorizing meanwhile does not affect the switch. Another account has another cloud generation, so the next pass
  merges the lists of this device and the new account without deleting anything.
- Disabling sync stops passes and forgets the account on this device. Access is never revoked: the grant belongs to
  the account, so revoking it would sign out every device of the account; the user can remove it in the Google
  account settings. Enabling sync starts from an empty base, after a pass that may still be running, so lists deleted
  while sync was off are not deleted in Drive. Data in Drive stays until the user taps "Delete data from Google Drive",
  which also disables sync on this device, forgets the base and deletes the `.generation` file first, then the other
  files, and forgets the account once they are deleted. If that fails, a toast shows the error and sync is enabled
  again, so the user can retry. Other devices with sync on see a new generation and upload their lists again, so
  clearing Drive needs sync off on them first.
- `play-services-auth` version: the newest one whose resolved `play-services-base`/`basement` still support
  minSdk 21 (21.4.0 at the time of writing). Pin it in `libs.versions.toml` with the usual
  `# Newer versions require minSdk 23` comment.

### REST calls

| Operation | Request |
|---|---|
| list | `GET files?spaces=appDataFolder&q='appDataFolder' in parents and mimeType != '<folder>'&fields=nextPageToken,files(id,name,md5Checksum,createdTime)&pageSize=1000` |
| generation, trash folder | `POST files` with metadata only |
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
- Quota: an idle pass costs one `files.list` (100 units), two once the `.Trash` folder exists (the purge lists it),
  each change 50–200 units; the free project limit is 400,000,000 units per day.

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
   While bookmark screens defer saving, the Java side reports the changes once notifications are enabled again,
   when core saves them. Changes during a pass, its own reloads and conflict copy renames included, queue one more
   pass after it (`APPEND_OR_REPLACE`): `REPLACE` would cancel the running pass, which has read the files already.
3. App foreground (`ProcessLifecycleOwner`, `KEEP`): pull remote changes on return.
4. "Sync now" in settings.
5. Closing the Place Page or a bookmarks screen, when the last pass postponed remote changes because of it (rule 3).

`BookmarksSyncWork` follows `app/src/main/java/app/organicmaps/background/OsmUploadWork.kt`:
- unique work `BookmarksSync`; every pass, "Sync now" included, needs `UNMETERED` unless "Sync over mobile data" is
  on (`CONNECTED`, or `NOT_ROAMING` when roaming is not accepted). While a pass waits for that on a metered network,
  settings show "Waiting for Wi-Fi", and without any network, a no-connection message;
- if WorkManager starts the process without an initialized core, the work returns `success()` and does nothing: the
  next app start runs a pass, and the snapshot model loses nothing;
- `SyncEngine` holds a `Mutex`, so passes never overlap.

| Work | Thread |
|---|---|
| network, hashing for the snapshot, base JSON | `Dispatchers.IO` |
| `ReloadBookmark`, category lookup by file, move to core `.Trash`, `areNotificationsEnabled`, Place Page selection, rename | `Dispatchers.Main` (core checks the thread in release too) |
| re-check MD5 and write or rename files in `bookmarks/` | core `Platform::Thread::File`, via `runOnFileThread` |

Core saves categories asynchronously on its File thread (`SaveBookmarks` → `WriteToTempAndRenameToFile`). Replacing a
file from another thread could let a queued save write old data over the downloaded version, and the next pass would
upload it back, reverting the remote edit. Running the re-check and rename on the same serial File thread orders them
after all queued saves. `ReloadBookmark` is then called on Main; it does not write the file back
(`CreateCategories` uses `NotifyChanges(false)`), so there is no echo.

Conflict copy sequence, in one File thread task: re-check the MD5, copy the local bytes to the new name through a
temporary file, and move the Drive version over the original. Then, on Main, reload the copy, rename its category and
reload the original. An interrupted pass leaves either both files changed or none, so it never makes a second copy.

Remaining races, both in the few milliseconds between the check that local changes can be applied and
`ReloadBookmark`: an edit of the same category wins over the Drive version (no data is lost locally; the remote edit
is overwritten by the next upload), and a Place Page opened on one of its bookmarks or tracks keeps an id that the
reload replaces.

## Core and JNI

New JNI in `android/sdk/src/main/cpp/app/organicmaps/sdk/bookmarks/data/BookmarkManager.cpp` and
`BookmarkManager.java`, reusing existing core functions:

| Java | Core |
|---|---|
| `nativeGetBookmarksDir()` | `GetBookmarksDirectory()` |
| `nativeGetCategoryIdByFile(path)` | `GetCategoryByFileName(path)`, `-1` if none |
| `nativeReloadCategoryFile(path)` | `ReloadBookmark(path)` |
| `nativeMoveCategoryToTrash(id)` | `DeleteBmCategory(id, false)`; the existing `nativeDeleteCategory` deletes permanently |
| `nativeIsBookmarkOrTrackSelected()` | `GetPlacePageInfo()`: `IsBookmark() \|\| IsTrack()` |
| `nativeRunOnFileThread(Runnable)` | `GetPlatform().RunTask(Platform::Thread::File, ...)`; the runnable must be short and must not call `BookmarkManager` |

The existing `BookmarkCategory` name API (`nativeSetName`) renames the copy.

## Settings UI and errors

- Privacy category: a "Google Drive sync (Beta)" item next to "Bookmarks backup", visible only when a provider is
  registered. It opens `BookmarksSyncSettingsFragment`:
  - "Sync bookmarks and tracks" switch;
  - status: "Last sync: <date and time>", "Sign-in required", "Google Drive is full", or an error message, with a
    spinner on the right while a pass runs ("Syncing…" on the first one), "Waiting for Wi-Fi" or a no-connection
    message under it (in its place before the first sync) while a pass waits for the network, and a "Sync now"
    button, disabled while a pass runs;
  - "Google Account" with the email: tap to switch the account, or to sign in when sign-in is required;
  - "Manage Google storage" with "<used> of <limit> used" (`about.get?fields=storageQuota`, binary units as Google
    shows them): opens Google One;
  - "Sync over mobile data" switch, off by default;
  - "Delete data from Google Drive", with confirmation.
- First enable, as on iOS: a "Beta, make a backup first" dialog with Cancel, Export KMZ (the existing export of all
  bookmarks) and Enable. Then authorization and the initial pass; an authorization error is shown as a toast.
- `SyncEngine` exposes `StateFlow<SyncStatus>`: `Idle(lastSync)`, `Running(lastSync)`, `NeedsSignIn`,
  `Error(kind)`. The last sync time is persisted. Transient network errors are not shown while WorkManager retries.
- All strings go to `data/strings/strings.txt`.

## Testing

- `:sdk:sync:core` JVM unit tests:
  - table tests of `Reconciler` for every row above, duplicate names and a name taken by a new file;
  - test cases ported from `iphone/Maps/Tests/Core/iCloudTests/` and #13232;
  - `SyncEngine` with a fake `CloudStore`, a temporary directory and a fake core port: action order, base persisted
    after each action, resuming an interrupted pass, skipping an action when the MD5 changed, generation changes
    (another account, data deleted elsewhere).
- `:sdk:sync:gms` tests with OkHttp `MockWebServer`: request shapes, pagination, resumable uploads, error mapping
  (401 retry, 403 quota, 429). `org.json:json` as a test dependency, because `android.jar` stubs are not usable in
  JVM tests.
- Core: `Bookmarks_RecentlyDeleted` in `libs/map/map_tests` also covers the category lookup by file path.
- CI (`.github/workflows/android-check.yaml`): PR checks do not build the Google flavor, so add
  `:sdk:sync:core:testDebugUnitTest`, `:sdk:sync:gms:testDebugUnitTest` and `:sdk:sync:gms:assembleDebug`.
  Kotlin modules get ktlint and detekt through `enableKotlin`.
- Manual, two devices with one test account: create, edit, conflict, delete, offline edits, account switch, deleting
  hidden app data in Drive settings.

## Risks and open questions

- Core `.Trash` on Android has no UI ("Recently deleted" exists on iOS only), so categories trashed by sync can be
  recovered only manually for now. A recovery UI can follow separately.
- The `play-services-auth` version is limited by minSdk 21; newer releases need minSdk 23.
- iOS joining later must use the same Google Cloud project to share `appDataFolder`, and decide how Drive sync
  coexists with iCloud sync.
