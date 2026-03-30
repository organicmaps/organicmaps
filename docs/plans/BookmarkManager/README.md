# BookmarkManager Architecture Analysis & Improvement Plans

## Overview

Detailed implementation plans for improving BookmarkManager architecture. Each plan is a separate file in this directory.

```
docs/plans/BookmarkManager/
  A-split-into-focused-components.md   [IN PROGRESS - BookmarkSorter done]
  B-unify-change-notification.md       [UPDATED with known bugs]
  C-extract-drape-communication.md
  D-simplify-edit-session.md
  E-unified-platform-bridge.md
  F-separate-system-marks.md
  G-async-data-access-android.md
  H-dead-code-cleanup.md               [UPDATED with new findings]
```

---

## Refactoring Progress

### Completed
- **BookmarkSorter extraction** (Plan A, step 1): Moved sorting logic, metadata persistence, and region address handling into `bookmark_sorter.hpp/cpp`. BookmarkManager delegates via `m_sorter` member with backward-compatible type aliases. Reduced BookmarkManager by ~800 lines. All tests pass.

### Deferred
- **BookmarkFileManager** and **TrackSelectionManager** extractions require internal interface extraction (`BookmarkDataProvider`, `UserMarkAccessor`) due to deep coupling with BookmarkManager's private APIs. See Plan A for details.

---

## Known Bugs in Change Tracking (MarksChangesTracker)

Code review uncovered several bugs and design issues in the change tracking system. These should be addressed **before or as part of** Plan B (Unify Change Notification).

### Bug 1: `HasBookmarksChanges()` ignores created categories

```cpp
bool MarksChangesTracker::HasBookmarksChanges() const
{
  return HasBookmarkCategories(m_updatedGroups) || HasBookmarkCategories(m_removedGroups);
  // BUG: does NOT check m_createdGroups
}
```

When a new category is created via `CreateBookmarkCategory()`, it goes into `m_createdGroups` via `OnAddGroup()`. But `HasBookmarksChanges()` only checks `m_updatedGroups` and `m_removedGroups`. This means:
- `NotifyBookmarksChanged()` is NOT called for new categories
- Auto-save in `NotifyChanges()` is NOT triggered for new categories
- `SendBookmarksChanges()` (platform callbacks) is NOT called

This is intentional for `LoadBookmark`/`ReloadBookmark` flows (which handle persistence separately), but creates inconsistency for `CreateBookmarkCategory()` and `CheckAndCreateDefaultCategory()` where the category exists in `m_categories` but may not have a file on disk.

**Impact**: A newly created category that has no bookmarks added to it may never be saved to disk until an explicit save is triggered through other means.

### Bug 2: Created categories may lack files on disk

`CreateBookmarkCategory(CategoryData &&, bool autoSave)` (the first overload) adds the category to `m_categories` and calls `m_changesTracker.OnAddGroup()`, but does NOT call `NotifyChanges()` directly. The category exists in memory but has no file.

The second overload `CreateBookmarkCategory(std::string const &, bool autoSave)` DOES call `NotifyBookmarksChanged()` and `NotifyCategoriesChanged()` directly, but these are the simple signal callbacks -- they don't trigger the auto-save path in `NotifyChanges()`.

If the app crashes after creating a category but before any bookmark is added to it, the category is lost.

**Potential conflict**: If a file with the same generated name is imported later (via `LoadBookmark`), there could be a naming conflict with the in-memory-only category.

### Bug 3: `AcceptDirtyItems()` overlaps `m_updatedGroups` with `m_createdGroups`

```cpp
void MarksChangesTracker::AcceptDirtyItems()
{
  CHECK(m_updatedGroups.empty(), ());
  m_bmManager->GetDirtyGroups(m_updatedGroups);  // Populates from IsDirty()

  for (auto const markId : m_updatedMarks)
  {
    if (mark->IsDirty())
      m_updatedGroups.insert(mark->GetGroupId());  // Can add IDs also in m_createdGroups
  }
}
```

After this call, `m_updatedGroups` may contain IDs that are also in `m_createdGroups`. The `m_drapeChangesTracker` uses `GetUpdatedGroupIds()` which returns this merged set, so Drape sees all dirty groups. But the semantic overlap is confusing and makes it hard to distinguish "newly created + modified" from "existing + modified". A separate `m_dirtyGroups` set for Drape would be cleaner.

### Bug 4: `HasCategoriesChanges()` is unused

`HasCategoriesChanges()` is only called from `NotifyChanges()` to decide whether to fire `NotifyCategoriesChanged()`. But the simple signal `m_categoriesChangedCallback` is not set by iOS at all, and on Android it triggers a cache update. The method checks `m_createdGroups || m_removedGroups` but not `m_updatedGroups` (e.g. category rename), making it incomplete for its stated purpose.

### Bug 5: `m_bookmarksChangedCallback` not used on iOS

`SetBookmarksChangedCallback` / `m_bookmarksChangedCallback` is set on Android (via JNI `OnBookmarksChanged()`) but not on iOS. The iOS bridge (`MWMBookmarksManager`) does not call `SetBookmarksChangedCallback`. This means the `NotifyBookmarksChanged()` callback is a no-op on iOS, and any behavior that depends on it is platform-inconsistent. Consider removing it and using the unified observer pattern (Plan B) instead.

### Bug 6: `LoadBookmark`/`ReloadBookmark` process files one at a time

The loading queue processes files sequentially with GUI -> File -> GUI -> File thread transitions per file:

```
GUI: LoadBookmark(file1) -> queue file2, file3
FILE: parse file1
GUI: CreateCategories(file1), pop queue, LoadBookmarkRoutine(file2)
FILE: parse file2
GUI: CreateCategories(file2), pop queue, LoadBookmarkRoutine(file3)
FILE: parse file3
GUI: CreateCategories(file3), m_asyncLoadingInProgress = false, fire onFinished
```

This is inefficient and risks UI/file state inconsistency when multiple files arrive simultaneously (e.g., cloud sync batch). Better to accept `std::vector<std::string>` and process all files in a single File thread task, then create all categories in a single GUI thread callback.

---

## A. Split BookmarkManager into Focused Components

### Status: IN PROGRESS

**BookmarkSorter** extracted (done). See `bookmark_sorter.hpp/cpp`.

**BookmarkFileManager** and **TrackSelectionManager** deferred -- require internal interface extraction. See `A-split-into-focused-components.md` for details.

---

## B. Unify Change Notification

### Problem (UPDATED)

The change tracking system has **6 known bugs** (see above) in addition to the complexity issues:
- 12+ callback mechanisms across 3 layers
- 3 parallel `MarksChangesTrackers` with complex merge/reset lifecycle
- `HasBookmarksChanges()` silently ignores created categories
- `m_updatedGroups` overlaps with `m_createdGroups` after `AcceptDirtyItems()`
- `HasCategoriesChanges()` is incomplete and mostly unused
- `m_bookmarksChangedCallback` not connected on iOS

### Proposed Solution (UPDATED)

**Step 0 (prerequisite bug fixes):**
1. Fix `HasBookmarksChanges()` to include `m_createdGroups` (or document why it intentionally excludes them and add a separate path for `CreateBookmarkCategory`)
2. Ensure `CreateBookmarkCategory()` first overload triggers proper save
3. Introduce a separate `m_dirtyGroups` set in `AcceptDirtyItems()` for Drape, keeping `m_updatedGroups` semantically clean
4. Remove `HasCategoriesChanges()` if truly not needed, or fix it to include `m_updatedGroups`
5. Remove `m_bookmarksChangedCallback` / `SetBookmarksChangedCallback` and `m_categoriesChangedCallback` / `SetCategoriesChangedCallback` (replace with observer pattern)

**Step 1: Reduce to 2 trackers** (see `B-unify-change-notification.md` for full details)

**Step 2: Unified observer interface** (replaces constructor callbacks + settable callbacks)

### Key Files
- `libs/map/bookmark_manager.hpp` -- callbacks, trackers, settable callbacks
- `libs/map/bookmark_manager.cpp` -- `NotifyChanges()`, `SendBookmarksChanges()`, `HasBookmarksChanges()`, `AcceptDirtyItems()`
- `libs/map/framework.cpp` -- where Callbacks struct is constructed

---

## C. Extract Drape Communication

BookmarkManager directly communicates with the rendering engine via `m_drapeEngine`, `m_drapeChangesTracker`, and `MarksChangesTracker` implementing `df::UserMarksProvider`. Extract into a `BookmarkRenderAdapter`.

See `C-extract-drape-communication.md`. Depends on Plan B.

---

## D. Simplify Edit Session

Replace 30+ forwarding proxy methods with a thin scope guard using `operator->()`. Mechanical refactor, low risk.

See `D-simplify-edit-session.md`. Independent of other plans.

---

## E. Unified Platform Bridge (C++ Side)

Create a C++ `BookmarkBridge` with DTOs to reduce iOS/Android bridge duplication.

See `E-unified-platform-bridge.md`. Independent, high effort.

---

## F. Separate System Marks

Move ephemeral system marks (`StaticMarkPoint`, `MyPositionMarkPoint`, `TrackSelectionMark`, etc.) to a `SystemMarkManager`.

See `F-separate-system-marks.md`. Benefits from Plan A.

---

## G. Async Data Access for Android

Add paginated query APIs to eliminate blocking JNI calls.

See `G-async-data-access-android.md`. Independent.

---

## H. Dead Code Cleanup

### Status: UPDATED with new findings

**Confirmed dead/unused:**
- `ExpiredCategory` struct + `m_expiredCategories` vector
- `GetSeacrhAPIFn` typo (should be `GetSearchAPIFn`)
- `HasCategoriesChanges()` -- only called once, incomplete logic (see Bug 4)
- `m_bookmarksChangedCallback` / `SetBookmarksChangedCallback` -- not used on iOS (see Bug 5)

**Confirmed still used (DO NOT remove):**
- `RestoringCache` / `m_restoringCache` -- used in `CreateCategories()` for access rules
- `m_lastCategoryUrl` -- persists last edited category to settings

See `H-dead-code-cleanup.md`.

---

## Implementation Priority & Dependencies (UPDATED)

```
Phase 1 (Quick wins):
  H. Dead Code Cleanup (including newly found dead code)
  D. Simplify Edit Session

Phase 2 (Bug fixes + foundational):
  B.0 Fix change tracking bugs (prerequisite for B)
  B. Unify Change Notification
  A. Continue component extraction (after defining internal interfaces)

Phase 3 (Depends on Phase 2):
  C. Extract Drape Communication (after B)
  F. Separate System Marks (after A)

Phase 4 (Independent, high effort):
  E. Unified Platform Bridge
  G. Async Data Access for Android

Phase 5 (File I/O improvements):
  Batch LoadBookmark/ReloadBookmark to accept file arrays (Bug 6)
```
