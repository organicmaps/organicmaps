# Bookmarks sync

Design for two-way synchronization of bookmarks and tracks between devices through a cloud store (#7408). The engine is
part of the C++ core; the first store is Google Drive on Android.

## Goals and non-goals

Goals:
- Keep bookmarks and tracks identical on all devices of one cloud account, without losing edits.
- One engine for all platforms and stores: the rules, conflicts and state live in the core, a store only moves files.
- Opt-in, Google flavor only for Drive. No Organic Maps servers, no secrets in the repository.
- Store data in the same per-category KML files that iOS iCloud sync uses, so iOS can join the same storage later.

Non-goals for the first version:
- iOS support, Android ↔ iOS sync, other stores (Proton Drive, Nextcloud #10199 and others can implement the store
  interface later).
- Real-time push of remote changes, periodic background sync while the app is not running.
- Selective sync, version history UI, syncing app settings or `bm.json` (category sorting).

The Android system Auto Backup (`android/app/src/main/res/xml/backup_content*.xml`) stays as is. It restores data
when the app is installed, but it is per device and does not sync.

## Decisions

| Topic | Decision |
|---|---|
| Engine | `libs/map/bookmarks_sync` in the core; stores are thin platform clients with blocking calls |
| Storage | Drive: hidden `appDataFolder`, scope `https://www.googleapis.com/auth/drive.appdata` (non-sensitive) |
| Unit of sync | One flat `*.kml` file in the bookmarks directory, identified by its file name |
| Change detection | Snapshot reconciliation by content: local hash, cloud version, last synced state (base) |
| Hashes | The store's algorithm when the core can compute it (Drive: MD5), BLAKE3 for stores without hashes |
| Conflicts | Edited on both sides → keep the local version as a copy named after the device model |
| Deletions | Soft on both sides: core `.Trash` locally, a trash of the store in the cloud |
| Local changes | Through core file updates guarded by category revisions, never behind a pending save or local edit |
| Flavors | Drive code in a module linked only by `googleImplementation`; no provider means no sync UI |

The reconciliation model follows the iOS rework in #13232 ("Reconcile iCloud files by content"), not the
mtime-based logic in `iphone/Maps/Core/iCloud/SynchronizationStateResolver.swift`.

## Architecture

- Core, `libs/map/bookmarks_sync`:
  - `Plan()` in `reconciler.cpp`: a pure function of the local files, the cloud files and the base.
  - `SyncEngine`: runs one pass at a time, applies the plan and persists the base after every action.
  - `LocalStore`: changes the bookmarks directory through `BookmarkManager` file updates (#13974) and checked
    deletions (#13972).
  - `CloudStore`: the interface that platforms implement.
- `:sdk`: the Kotlin `CloudStore` interface, `BookmarksSyncEngine` over JNI and `CloudSyncRegistry` for account
  handling (`CloudSyncProvider`).
- `:sdk:sync:gms`, linked with `googleImplementation` only: Drive REST v3 over OkHttp + org.json and
  play-services-auth authorization. A manifest `ContentProvider` registers the provider, like `WearBridgeInitProvider`.
- `:app`: `BookmarksSyncWork` (WorkManager), `BookmarksSync` (triggers, the gate for open screens) and the settings
  screen.

Without `:sdk:sync:gms` (fdroid, web, huawei) the registry is empty, so the settings item is hidden and nothing runs.

### Store interface

A store reports its hash algorithm, or none, and offers blocking calls that throw `CloudException`:

| Call | Meaning |
|---|---|
| `List()` | Top-level files without folders and trashed files, the `.generation` files included; may lag behind |
| `Stat(id)` | The file, or nothing once it is deleted or trashed; never lags behind |
| `CreateGeneration()` | Creates an empty `.generation` file |
| `Download(id, path)`, `Upload(name, path, existingId)` | Content transfer; an upload with an id replaces its content |
| `MoveToTrash(id)`, `PurgeTrash(before)` | Soft deletion and cleanup of the store's own trash |
| `DeleteAll(generation)` | Deletes the generation first, then the other generations and files |

A cloud file has an id, a name, a creation time, a version that changes with every content change, and an optional
content hash. For Drive the version and the hash are `md5Checksum`. Selecting the generation, filtering `*.kml` files
and files created before the generation, duplicates and all re-checks are in the core.

## Local state

- Bookmarks directory: `GetBookmarksDirectory()`. Only regular `*.kml` files at the top level are synced, names with a
  leading dot included: core names the file after the list. Core temporary files (`*.kml.tmp*`) are ignored.
- Base state: `getFilesDir()/sync/google_drive.json` with the hash algorithm, the cloud generation, the last sync and
  purge times, the cloud id, local hash and cloud version of every synced file and the hash of uploads not recorded
  yet, written with temp file + rename. Version 1, written by the first Kotlin engine with MD5 only, is still read. A
  base of another algorithm is ignored.
- Staged files: `getFilesDir()/sync/tmp`, immutable copies that are uploaded or that core parses, deleted with their
  last reference and when the engine starts.
- The base is outside `bookmarks/`, so Auto Backup does not restore it. A device restored from Auto Backup starts with
  an initial reconciliation, and identical files are matched by hash without copies.
- The hash detects changes only; it is not a security measure.

## Reconciliation

For every file name the pass knows the local hash L, the cloud file C with its version and hash, and the base entry B
with the cloud id, the local hash and the cloud version of the last sync. L changed when its hash differs from B; C
changed when its version differs from B; L and C are equal when their hashes are. A store without hashes gives C the
hash of B while its id and version match B, otherwise the core downloads C first and hashes it.

A base entry applies only to the cloud file it was recorded for. When another file owns the name now and the synced
one is still listed, this device's upload lost a duplicate race, and the name is reconciled as if it had no base
entry. When the synced one is not listed, it was deleted elsewhere and a new file took its name: an unchanged local
file (L = B) is replaced by the new one once `Stat` confirms the deletion (rule 2), otherwise the name is reconciled as
if it had no base entry.

Files with a base entry:

| L | C | Action |
|---|---|---|
| = B | = B | nothing |
| ≠ B | = B | upload L |
| = B | ≠ B | download C, replace local file, reload category |
| ≠ B | ≠ B, L = C | update base only |
| ≠ B | ≠ B, L ≠ C | conflict: local version becomes a copy, cloud version replaces the original name |
| missing | = B | deleted locally: move the cloud file to the trash |
| missing | ≠ B | deleted here, edited elsewhere: download C (an edit wins over a deletion) |
| = B | missing | deleted elsewhere: move local category to core `.Trash` once the cloud confirms it (rule 2) |
| ≠ B | missing | deleted elsewhere, edited here: upload L (over the synced cloud file if it still exists, rule 2) |
| missing | missing | trash the synced cloud file if `Stat` still finds it unchanged, then drop the base entry |

Files without a base entry (new files, initial sync, after a reset):

| L | C | Action |
|---|---|---|
| present | missing | upload L |
| missing | present | download C |
| present | present, L = C | record base |
| present | present, L ≠ C | conflict as above, except that an untouched list yields to the other version without a copy |

An untouched list has no bookmarks, tracks, description and annotation, like the default "My Places" of a new device.

Conflict copy:
- File name `<base> (<device>).kml`, built with the core rules (`RemoveInvalidSymbols`, `TruncateToValidFileName`),
  unique among the local files, the loaded categories and the cloud files of the pass. The device name is
  `Settings.Global "device_name"`, falling back to `Build.MODEL`.
- The category inside the copy is renamed to `<name> (<device>)` in the same file update that installs the cloud
  version, so an interrupted pass never leaves one without the other.
- The copy is uploaded as a new file by the next pass.

Safety rules:
1. Cloud generation: the cloud holds an empty `.generation` file, and the base records its id. Another account has
   another generation; deleting all data from the app or hidden app data in Drive settings deletes it, and the next
   pass creates a new one (when two devices create it at once, the earliest creation time, then the smallest id,
   wins). A listing without a generation creates one only once `Stat` confirms that the base generation is gone. Only
   files created since the generation are reconciled: older ones are left from deleted data, which another device may
   still be deleting one by one, so lists are uploaded again rather than matched with them. If the generation differs
   from the base, the base is cleared and the pass runs as an initial reconciliation, so nothing is deleted locally
   because of it. Within one generation, a file missing in the cloud was deleted on purpose.
2. Re-check before destructive actions: a local file is replaced or trashed only if its hash is still the one the plan
   saw, and only through a file update that core refuses after a save request or an edit of its category; before a
   cloud file is overwritten or trashed, `Stat` returns its current version. If either changed, the action is skipped
   and the pass ends with Retry, so another pass follows soon. Duplicates (rule 4) are trashed without a re-check, and
   the device that keeps writing one sees that it lost the name. Listings can lag behind, while `Stat` can't: a file
   missing from the listing trashes the local category only once `Stat` reports it gone and the generation still
   there; if the generation is gone, the pass stops and the next one sees the new generation.
3. No local apply while it would lose edits or break a screen: core refuses file updates while bookmark screens keep
   edits unsaved (notifications are off), and a platform gate refuses them while a screen keeps category ids. On
   Android these are the bookmarks screens (`BaseBookmarksActivity`), the list settings screen and the Place Page
   showing a bookmark or track. Core checks the revision and the gate again in the GUI task that applies the parsed
   cloud version; a refused update is kept in the cloud and reconciled by a later pass. Downloads, replacements and
   local trash are postponed then, uploads continue, the last sync time stays as it was, and the pass runs again once
   the Place Page or the bookmarks screen is closed, or after the WorkManager retry delay while it stays open.
4. Duplicate names in the cloud (two devices created `Paris.kml` before seeing each other): the file with the earliest
   creation time (then the smallest id) owns the name. A duplicate that the base refers to with its synced version is
   the version this device synced last (its upload lost the name): the action for the name keeps it as a conflict
   copy, matches it, or replaces it with an edit or a deletion made here, and then the duplicate is moved to the trash
   without a copy. Until that action is applied, the duplicate waits in the cloud. Other duplicates with the same
   content as the owner are moved to the trash; the rest are downloaded under a unique local name and then moved to
   the trash. All devices apply the same rule, so the state converges.
5. Order inside a pass: conflict copies, downloads, uploads, deletions, duplicates. The base is persisted after every
   successful action, so an interrupted pass continues safely next time. An upload sends a copy of the local file
   staged after the pending saves, and the hash of that copy is persisted before the upload starts: when the upload
   reaches the cloud but its result is lost (the pass is cancelled, the process killed or the connection drops), the
   next pass finds the cloud file with that name and hash and records it as synced, instead of taking this device's own
   version for a conflicting one.
6. Trashed cloud files are deleted permanently 30 days after they were trashed. The purge runs at most once a day:
   listing the trash costs as much quota as the rest of an idle pass. Drive purges every `.Trash` folder, whichever
   device created it.

Known effects:
- Category visibility is stored in KML, so hiding a list is synced too.
- No-op rewrites by core (address updates after sorting by distance, saving unchanged data) do not change the hash and
  cause no uploads.
- When another device downloads a duplicate (rule 4) before the device that uploaded it resolves the name, the
  version that lost the name can end up in two lists: the conflict copy and the downloaded duplicate. Nothing is lost.

## Google Drive

### Authorization

- `AuthorizationClient.authorize(...)` with the `drive.appdata` scope. No Google Sign-In, no client id in the code,
  no backend: the Android OAuth client is matched by package name and signing SHA-1. Background passes never show UI:
  a required resolution shows "Sign-in required" in the settings.
- The synced account is stored by the provider and passed to every `authorize()`, so background passes never pick
  another one. A switched account becomes the synced one only once authorized; it has another cloud generation, so the
  next pass merges the lists of this device and the new account without deleting anything.
- Disabling sync forgets the account on this device. Access is never revoked: the grant belongs to the account, so
  revoking it would sign out every device of the account. Enabling sync keeps the base only for the lists that exist
  on this device: lists edited while sync was off are uploaded without conflict copies, and lists deleted meanwhile
  are downloaded again instead of deleted in Drive.
- "Delete data from Google Drive" disables sync on this device, deletes the base's `.generation` file by id (a
  listing can lag behind), then the other files, and forgets the base last. On failure sync is on again and a pass
  starts, which either finds the base still matching the Drive files or starts a new generation, so a failure
  neither copies nor deletes a list. Other devices with sync on see a new generation and upload their lists again,
  so clearing Drive needs sync off on them first.

### REST calls

- Uploads are always resumable: multipart is limited to 5 MB and recorded tracks can be larger.
- Files inside `appDataFolder` cannot be trashed with the Drive trash, hence the `.Trash` folder: a trashed file moves
  there with `appProperties.trashedAt`. The oldest `.Trash` folder is used, and one is created on the first deletion.
- `.generation` files are created with metadata only, so they have no `md5Checksum`.
- Errors: `403 storageQuotaExceeded` stops the pass with a "Google Drive is full" status; `429`,
  `403 rateLimitExceeded`/`userRateLimitExceeded`, `5xx` and network errors return `Result.retry()` (WorkManager
  exponential backoff); other `4xx` set an error status with the message. HTTP 401 clears the token and retries once.
- Cancelling a pass aborts its requests; requests of the settings screen continue.
- Quota: an idle pass costs one `files.list` (100 units), plus a few more once a day for the purge, each change
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
6. A pass that ended with Retry, after the WorkManager retry delay.

Every pass needs `UNMETERED` unless "Sync over mobile data" is on. If WorkManager starts the process without an
initialized core, the work does nothing: the next app start runs a pass, and the snapshot model loses nothing.

The engine runs on the worker thread and blocks it, calling the store there; it never runs on the GUI thread, which it
waits for. Core saves categories asynchronously on its File thread. A snapshot first waits for the saves queued so
far, so a pass started right after a change sees it. A local replacement takes a file update on the GUI thread, which
core refuses while saves are pending, checks the hash and writes the file in a File thread task that excludes saves
of the category, then reloads the category from the staged copy; core applies it on the GUI thread only if the
category revision, the notifications and the gate still allow it, otherwise it saves the category over the file
again. A reload does not write the file back (`CreateCategories` uses `NotifyChanges(false)`). Core still reports
reloaded and trashed categories as changes, which queues one more pass; it finds nothing to sync, so there is no
echo. Waits for the GUI and File threads give up after 10 s if the task has not started, for example while core tears
down.

## Risks and open questions

- Two devices that both overwrite the same unchanged cloud file between their re-checks and uploads both succeed, and
  the first one later downloads the second one's version without a conflict copy. Drive v3 has no conditional upload;
  preserving both versions needs another mechanism, for example Drive revisions.
- Core `.Trash` on Android has no UI ("Recently deleted" exists on iOS only), so categories trashed by sync can be
  recovered only manually for now. A recovery UI can follow separately.
- Android Auto screens keep category and bookmark ids outside the gate of rule 3.
- The `play-services-auth` version is limited by minSdk 21; newer releases need minSdk 23.
- iOS joining later must use the same Google Cloud project to share `appDataFolder`, and decide how Drive sync
  coexists with iCloud sync.
