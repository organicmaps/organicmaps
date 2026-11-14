# One-Shot Contact Import

Local branch: `feature/android-contact-import-clean`, based on upstream master
`0297791ee7b92105655f4573b29c16a02737b825`. This does not modify PR #13311 or #13716.
The previous prototype remains on `feature/android-contact-import` in its original checkout.

## Behavior

- Bookmarks and Tracks > Import contacts asks for explicit confirmation and, if needed,
  Android Contacts access. Android 5 does not receive this permission or show this action.
- Only postal-address rows, names, and provider identifiers needed for deduplication are read.
  Reads run off the UI thread and are cancellable. Nothing observes or synchronizes contacts.
- Addresses are resolved offline against downloaded maps, including places outside the visible
  viewport. Only unique mapped addresses or OSM address interpolation are accepted, not guessed
  nearby buildings. Primary addresses are tried before other addresses; each contact gets at most
  one bookmark. An address shared by several people is resolved once per import.
- The tagged Contacts list is reused even after renaming or hiding it. An unrelated list named
  Contacts is left alone. Moved imported bookmarks are found in their current lists.
- Re-import updates bookmarks by hashed provider identity. An unambiguous name/address fallback
  can recover changed provider keys. Ambiguous duplicates are skipped rather than overwritten.
- User changes to names, descriptions, or locations are preserved. Color/icon and list visibility
  are not reset. The import does not consume the place-page bookmark undo state.
- Missing or deleted source contacts do not delete existing bookmarks. Unresolved addresses are
  skipped and reported. Re-import retries them; unchanged source addresses reuse the source
  coordinates already recorded in their imported bookmarks, without another persistent cache.
- Temporary contact data is released on completion, cancellation, or failure. Cancellation before
  the saving phase makes no bookmark changes. Rotation retains progress; abandoning the screen
  no longer cancels the task. A user-started foreground service shows progress and Cancel
  in a notification (when notification permission is granted). Hide the progress
  dialog to browse the map; Import contacts reopens progress without starting a second import.
  Android may still terminate work, or the user may force-stop it. A process restart offers an
  explicit retry of an interrupted import, never automatic contact access. No address-book
  checkpoint is stored. Completed imports remain bookmarks. Normal list visibility/deletion
  provides hiding/removal; this feature does not claim to undo updates to existing bookmarks.

Imported entries are ordinary user-owned bookmarks. They remain after Contacts access is revoked
and are included in normal bookmark backups/exports. Hashed keys are identifiers, not encryption
or a claim of anonymity. KML/KMB preserve import metadata for re-import after a restore; formats
that discard custom bookmark metadata cannot provide that guarantee.

The shared address-search prerequisite is reused from the separately reviewed search work.
There is no contact-specific map marker layer, search provider, setting, or provider observer.
Global Android search queries the existing bookmark-name index across all lists, alongside
ordinary map search. Imported contacts use that same bookmark path without Contacts access.
The familiar person symbol and label presentation use generic bookmark custom-image metadata.
User-selected icons/colors override the imported presentation. Existing untouched imports gain
the presentation on re-import. The shared prerequisite also improves ordinary address matching.

## Local Test Build

The OM Test artifact uses `app.organicmaps.cyclingtests.debug`, the existing debug signing key,
and the local `OM Test` label override. It does not replace `app.organicmaps.debug` (OM Contacts).
Local packaging is selected with the init script outside this checkout:

```sh
cd android
ANDROID_HOME="$HOME/Android/Sdk" ./gradlew app:assembleGoogleDebug \
  --init-script /home/jared/Repos/organicmaps/contacts-import-test.init.gradle \
  -Parm64 -Pnjobs=1 --max-workers=1
```

## Manual Checks

1. Import with maps for the relevant cities downloaded. Open the resulting Contacts list.
2. Re-import unchanged contacts: no extra pins or entries should be created.
3. Change a source contact's address/name, then re-import: its bookmark should update in place.
4. Edit an imported bookmark's name or location, then change that contact and re-import:
   the bookmark should be reported as locally edited and kept unchanged.
5. Move an imported bookmark to another list; re-import must not create another copy.
6. Rename/hide the Contacts list; re-import must preserve those choices.
7. Cancel while reading/resolving; no new bookmarks should be saved. Rotate during import.
8. Deny/revoke Contacts access. No reads or automatic imports should happen at startup or
   during ordinary map/search use. Imported bookmarks should remain available.
9. Include missing addresses and contacts with multiple postal rows; check skip counts and
   that only one resolvable address is imported per person.

## Automated Checks

Android tests cover address preparation, grouping/deduplication, cancelled/stale callbacks,
permission revocation, per-import lookup reuse, invalid coordinates, and source-key hashing.
Native tests cover idempotent updates, user edits/moves, unrelated categories, invalid input,
key recovery, serialization rounding, KML/KMB round-trips, undo, and hidden/renamed lists.
The search integration test distinguishes deadline-limited results from complete address lookups.

```sh
cmake -S . -B build-contact-import-native -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DSKIP_QT_GUI=ON -DBUILD_DEV_SANDBOX=OFF -DBUILD_OPENGL_TESTS=OFF
cmake --build build-contact-import-native --target map_tests search_tests search_integration_tests --parallel 1
```

Native tests must use isolated writable/settings directories, not personal bookmark data.
No GitHub publication or PR changes are part of this local test preparation.
