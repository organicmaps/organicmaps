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

- `:sdk`: JNI over existing core functions (`ReloadBookmark`, `GetCategoryByFileName`, `DeleteBmCategory(id, false)`,
  `GetBookmarksDirectory`).
- `:sdk:sync:core`, all flavors, no Google dependencies: `Reconciler` (pure function local, cloud, base → actions),
  `LocalStore`, `BaseStateStore`, `SyncEngine`, the `CloudStore` interface and `CloudSyncRegistry`.
- `:sdk:sync:gms`, linked with `googleImplementation` only: Drive REST v3 over OkHttp + org.json and
  play-services-auth authorization. A manifest `ContentProvider` registers the provider, like `WearBridgeInitProvider`.
- `:app`: `BookmarksSyncWork` (WorkManager) and the settings screen.

Without `:sdk:sync:gms` (fdroid, web, huawei) the registry is empty, so the settings item is hidden and nothing runs.
Another provider (for example Nextcloud, #10199) can implement `CloudStore` in its own module.

## Local state

- Bookmarks directory: `GetBookmarksDirectory()`, which is `getFilesDir()/bookmarks` on Android. Only regular
  `*.kml` files at the top level are synced, names with a leading dot included: core names the file after the list.
  Core temporary files (`*.kml.tmp*`) are ignored.
- Base state: `getFilesDir()/sync/google_drive.json` with the cloud generation, the `.Trash` folder id, the last sync
  and purge times and the Drive id and MD5 of every synced file, written with temp file + rename.
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
   and the edit dialog opened from it hold their ids too. A reload or trash of their category replaces those ids. The
   list settings screen (`BookmarkCategorySettingsActivity`) holds the id of its category, and core aborts on it once
   the category is trashed.
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
6. Drive `.Trash` files are deleted permanently 30 days after they were trashed (`appProperties.trashedAt`). The
   purge runs at most once a day: listing the folder costs as much quota as the rest of an idle pass.

Known effects:
- Category visibility is stored in KML, so hiding a list is synced too.
- No-op rewrites by core (address updates after sorting by distance, saving unchanged data) do not change the MD5
  and cause no uploads.

## Google Drive

### Authorization

- `AuthorizationClient.authorize(...)` with the `drive.appdata` scope. No Google Sign-In, no client id in the code,
  no backend: the Android OAuth client is matched by package name and signing SHA-1. Background passes never show UI:
  a required resolution shows "Sign-in required" in the settings.
- The synced account is stored by the provider and passed to every `authorize()`, so background passes never pick
  another one. A switched account becomes the synced one only once authorized; it has another cloud generation, so the
  next pass merges the lists of this device and the new account without deleting anything.
- Disabling sync forgets the account on this device. Access is never revoked: the grant belongs to the account, so
  revoking it would sign out every device of the account. Enabling sync starts from an empty base, so lists deleted
  while sync was off are not deleted in Drive.
- "Delete data from Google Drive" disables sync on this device, forgets the base and deletes the `.generation` file
  first, then the other files. Other devices with sync on see a new generation and upload their lists again, so
  clearing Drive needs sync off on them first.

### REST calls

- Uploads are always resumable: multipart is limited to 5 MB and recorded tracks can be larger.
- Files inside `appDataFolder` cannot be trashed with the Drive trash, hence the `.Trash` folder, created on the first
  deletion.
- Errors: `403 storageQuotaExceeded` stops the pass with a "Google Drive is full" status; `429`,
  `403 rateLimitExceeded`/`userRateLimitExceeded`, `5xx` and network errors return `Result.retry()` (WorkManager
  exponential backoff); other `4xx` set an error status with the message. HTTP 401 clears the token and retries once.
- Quota: an idle pass costs one `files.list` (100 units), plus one more once a day for the purge, each change
  50–200 units. Above 400,000,000 units per day per project, Google plans to bill the project; this threshold can't
  be increased.

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
1. App start: the first load of bookmarks reports its categories as changes (trigger 2), and the app comes to the
   foreground (trigger 3). A pass that starts while core is still loading bookmarks does nothing.
2. Local changes, 5 s after the last one, so a burst of edits produces one pass. Changes deferred by bookmark screens
   are reported when core saves them. Changes during a pass queue one more pass after it: `REPLACE` would cancel the
   running pass, which has read the files already.
3. App foreground: pull remote changes on return.
4. "Sync now" in settings.
5. Closing the Place Page or a bookmarks screen, when the last pass postponed remote changes because of it (rule 3).

Every pass needs `UNMETERED` unless "Sync over mobile data" is on. If WorkManager starts the process without an
initialized core, the work does nothing: the next app start runs a pass, and the snapshot model loses nothing.

Core saves categories asynchronously on its File thread (`SaveBookmarks` → `WriteToTempAndRenameToFile`). Replacing a
file from another thread could let a queued save write old data over the downloaded version, and the next pass would
upload it back, reverting the remote edit. Running the re-check and rename on the same serial File thread orders them
after all queued saves. The snapshot of a pass waits for the queued saves too, so a pass started right after a change
sees it. `ReloadBookmark` is then called on Main; it does not write the file back (`CreateCategories` uses
`NotifyChanges(false)`), and the changes that core reports for reloaded and trashed categories don't trigger a pass,
so there is no echo.

Remaining races, both in the few milliseconds between the check that local changes can be applied and
`ReloadBookmark`: an edit of the same category wins over the Drive version (no data is lost locally; the remote edit
is overwritten by the next upload), and a Place Page opened on one of its bookmarks or tracks keeps an id that the
reload replaces. A change that core reports while a pass reloads or trashes a category triggers no pass; the next
trigger syncs it.

## Risks and open questions

- Core `.Trash` on Android has no UI ("Recently deleted" exists on iOS only), so categories trashed by sync can be
  recovered only manually for now. A recovery UI can follow separately.
- The `play-services-auth` version is limited by minSdk 21; newer releases need minSdk 23.
- iOS joining later must use the same Google Cloud project to share `appDataFolder`, and decide how Drive sync
  coexists with iCloud sync.
