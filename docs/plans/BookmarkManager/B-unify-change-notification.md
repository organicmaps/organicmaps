# B. Unify Change Notification

## Problem

BookmarkManager has 12+ callback mechanisms across 3 layers, 3 parallel change trackers, and **6 known bugs** in the change tracking system.

### Known Bugs (from code review)

**Bug 1: `HasBookmarksChanges()` ignores created categories**
```cpp
// bookmark_manager.cpp
bool MarksChangesTracker::HasBookmarksChanges() const
{
  return HasBookmarkCategories(m_updatedGroups) || HasBookmarkCategories(m_removedGroups);
  // BUG: does NOT check m_createdGroups
}
```
When `CreateBookmarkCategory()` adds a group via `OnAddGroup()`, it goes into `m_createdGroups` only. `NotifyChanges()` uses `HasBookmarksChanges()` to gate auto-save and `SendBookmarksChanges()`, so newly created categories are silently skipped. This is intentional for `LoadBookmark`/`ReloadBookmark` (which handle persistence separately), but creates inconsistency for user-initiated category creation.

**Bug 2: Created categories may lack files on disk**
- `CreateBookmarkCategory(CategoryData &&, bool autoSave)` does NOT call `NotifyChanges()`
- `CreateBookmarkCategory(std::string const &, bool autoSave)` calls `NotifyBookmarksChanged()` + `NotifyCategoriesChanged()` directly, but these are simple signal callbacks -- they don't trigger the auto-save path
- Result: category exists in `m_categories` with no file on disk
- Risk: app crash loses the category; imported file with same name may conflict

**Bug 3: `AcceptDirtyItems()` overlaps `m_updatedGroups` with `m_createdGroups`**
```cpp
void MarksChangesTracker::AcceptDirtyItems()
{
  CHECK(m_updatedGroups.empty(), ());
  m_bmManager->GetDirtyGroups(m_updatedGroups);  // from IsDirty()

  for (auto const markId : m_updatedMarks)
    if (mark->IsDirty())
      m_updatedGroups.insert(mark->GetGroupId());  // can overlap with m_createdGroups
}
```
After this, `m_updatedGroups` may contain IDs also present in `m_createdGroups`. The Drape tracker uses `GetUpdatedGroupIds()` which returns this merged set. A separate `m_dirtyGroups` set for Drape would be semantically cleaner.

**Bug 4: `HasCategoriesChanges()` is incomplete and nearly unused**
- Only called from `NotifyChanges()` to gate `NotifyCategoriesChanged()`
- Checks `m_createdGroups || m_removedGroups` but NOT `m_updatedGroups` (misses renames)
- `m_categoriesChangedCallback` is not set on iOS, so the gated callback is only used on Android

**Bug 5: `m_bookmarksChangedCallback` not used on iOS**
- `SetBookmarksChangedCallback` is called on Android (JNI `OnBookmarksChanged()`) but not on iOS
- `NotifyBookmarksChanged()` is a no-op on iOS

**Bug 6: `LoadBookmark`/`ReloadBookmark` process files one at a time**
The loading queue processes files sequentially with GUI -> File -> GUI -> File thread transitions per file. When multiple files arrive simultaneously (cloud sync, multi-file import), this creates:
- Unnecessary thread ping-pong (O(n) GUI<->File transitions instead of O(1))
- Window where `m_categories` state is partially updated (some files loaded, others still in queue)
- UI updates between each file load instead of a single batch update

Better API: `LoadBookmarks(std::vector<std::string> const & filePaths, bool isTemporaryFile)` to process all files in a single File thread task.

### Current Callback Inventory

**Layer 1 -- Constructor callbacks** (permanent, set once in Framework):
- `m_createdBookmarksCallback` -- `std::function<void(vector<BookmarkInfo> const &)>`
- `m_updatedBookmarksCallback` -- same signature
- `m_deletedBookmarksCallback` -- `std::function<void(vector<kml::MarkId> const &)>`
- `m_attachedBookmarksCallback` -- `std::function<void(vector<BookmarkGroupInfo> const &)>`
- `m_detachedBookmarksCallback` -- same signature

**Layer 2 -- Settable callbacks** (set by platform bridges):
- `m_bookmarksChangedCallback` -- not used on iOS (Bug 5)
- `m_categoriesChangedCallback` -- not used on iOS
- `m_elevationActivePointChanged` -- used on both platforms
- `m_elevationMyPositionChanged` -- used on both platforms
- `m_onSymbolSizesAcquiredFn` -- internal use only

**Layer 3 -- Async loading callbacks** (set by platform bridges):
- `m_asyncLoadingCallbacks.m_onStarted`
- `m_asyncLoadingCallbacks.m_onFinished`
- `m_asyncLoadingCallbacks.m_onFileSuccess`
- `m_asyncLoadingCallbacks.m_onFileError`

### Current Tracker Flow

Three `MarksChangesTracker` instances:
```
m_changesTracker           -- accumulates during edit sessions
m_bookmarksChangesTracker  -- merged, drives persistence + platform callbacks
m_drapeChangesTracker      -- merged, drives rendering updates
```

`NotifyChanges()` flow:
```
1. m_changesTracker.AcceptDirtyItems()
2. if HasBookmarksChanges -> NotifyBookmarksChanged()    [BUG: skips created]
3. if HasCategoriesChanges -> NotifyCategoriesChanged()  [BUG: incomplete]
4. m_bookmarksChangesTracker.AddChanges(m_changesTracker)
5. m_drapeChangesTracker.AddChanges(m_changesTracker)
6. m_changesTracker.ResetChanges()
7. if bookmarks changed:                                 [BUG: skips created]
   a. Collect dirty categories -> SaveBookmarks()
   b. SendBookmarksChanges() -> 5 constructor callbacks
8. m_bookmarksChangesTracker.ResetChanges()
9. if drape changes:
   a. Lock DrapeEngine
   b. Update visibility, push marks, invalidate
10. m_drapeChangesTracker.ResetChanges()
```

## Proposed Solution

### Step 0: Fix change tracking bugs (prerequisite)

1. **Fix `HasBookmarksChanges()`**: Either include `m_createdGroups` in the check, or create a separate `HasCreatedCategoriesChanges()` method and add an explicit save path for new categories in `NotifyChanges()`.

2. **Fix category file creation**: Both `CreateBookmarkCategory` overloads should ensure the category is saved (or queued for save) before returning. The simplest fix: call `NotifyChanges(true)` at the end of both overloads.

3. **Separate dirty groups for Drape**: In `AcceptDirtyItems()`, populate a dedicated `m_dirtyGroupsForDrape` set that includes both created and updated groups. Keep `m_updatedGroups` semantically clean (only existing groups that changed).

4. **Fix or remove `HasCategoriesChanges()`**: Either add `m_updatedGroups` check (for renames) or remove it entirely and always call `NotifyCategoriesChanged()` when there are any group changes.

5. **Remove unused simple callbacks**: Remove `m_bookmarksChangedCallback`/`SetBookmarksChangedCallback` and `m_categoriesChangedCallback`/`SetCategoriesChangedCallback`. Replace with unified observer pattern in Step 2.

### Step 1: Eliminate one tracker

Replace 3 trackers with 2. See original proposed `NotifyChanges()` rewrite below, **updated** to address Bug 1:

```cpp
void BookmarkManager::NotifyChanges(bool saveChangesOnDisk)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_changesTracker.AcceptDirtyItems();

  if (!m_firstDrapeNotification && !m_changesTracker.HasChanges()
      && !m_drapeChangesTracker.HasChanges())
    return;

  // 1. Persistence + detailed callbacks (from m_changesTracker)
  if (!m_notificationsEnabled)
  {
    m_drapeChangesTracker.AddChanges(m_changesTracker);
    m_changesTracker.ResetChanges();
    return;
  }

  // FIX Bug 1: check created OR updated OR removed for save decision
  bool const hasBookmarkGroupChanges =
    m_changesTracker.HasBookmarksChanges() ||
    HasBookmarkCategories(m_changesTracker.GetCreatedGroupIds());

  if (hasBookmarkGroupChanges)
  {
    if (saveChangesOnDisk)
    {
      kml::GroupIdCollection categoriesToSave;
      // Save both updated and newly created categories
      for (auto groupId : m_changesTracker.GetUpdatedGroupIds())
        if (IsBookmarkCategory(groupId) && GetBmCategory(groupId)->IsAutoSaveEnabled())
          categoriesToSave.push_back(groupId);
      for (auto groupId : m_changesTracker.GetCreatedGroupIds())
        if (IsBookmarkCategory(groupId) && GetBmCategory(groupId)->IsAutoSaveEnabled())
          categoriesToSave.push_back(groupId);
      SaveBookmarks(categoriesToSave);
    }

    SendBookmarksChanges(m_changesTracker);
  }

  // 2. Drape update
  m_drapeChangesTracker.AddChanges(m_changesTracker);
  m_changesTracker.ResetChanges();

  if (!m_drapeChangesTracker.HasChanges())
    return;

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    auto engine = lock.Get();
    for (auto groupId : m_drapeChangesTracker.GetUpdatedGroupIds())
      engine->ChangeVisibilityUserMarksGroup(groupId, GetGroup(groupId)->IsVisible());
    for (auto groupId : m_drapeChangesTracker.GetRemovedGroupIds())
      engine->ClearUserMarksGroup(groupId);
    engine->UpdateUserMarks(&m_drapeChangesTracker, m_firstDrapeNotification);
    m_firstDrapeNotification = false;
    engine->InvalidateUserMarks();
  }
  m_drapeChangesTracker.ResetChanges();
}
```

### Step 2: Unified observer interface

```cpp
class BookmarkObserver
{
public:
  virtual ~BookmarkObserver() = default;
  virtual void OnBookmarksChanged() {}
  virtual void OnCategoriesChanged() {}
  virtual void OnBookmarksCreated(std::vector<BookmarkInfo> const &) {}
  virtual void OnBookmarksUpdated(std::vector<BookmarkInfo> const &) {}
  virtual void OnBookmarksDeleted(std::vector<kml::MarkId> const &) {}
  virtual void OnBookmarksAttached(std::vector<BookmarkGroupInfo> const &) {}
  virtual void OnBookmarksDetached(std::vector<BookmarkGroupInfo> const &) {}
};

void AddObserver(BookmarkObserver & observer);
void RemoveObserver(BookmarkObserver & observer);
```

### Step 3: Batch file loading (fix Bug 6)

```cpp
// Replace single-file API:
void LoadBookmark(std::string const & filePath, bool isTemporaryFile);

// With batch API:
void LoadBookmarks(std::vector<std::string> const & filePaths, bool areTemporaryFiles);
```

Implementation processes all files in a single File thread task, creates all categories in a single GUI thread callback, fires a single `onFinished` notification.

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Fix `HasBookmarksChanges()`, remove `m_bookmarksChangesTracker`, add `BookmarkObserver`, add batch `LoadBookmarks()` |
| `libs/map/bookmark_manager.cpp` | Rewrite `NotifyChanges()`, fix `AcceptDirtyItems()`, update `CreateBookmarkCategory()`, rewrite loading queue for batch processing |
| `libs/map/framework.cpp` | Implement `BookmarkObserver` instead of 5 constructor callbacks |
| `iphone/CoreApi/CoreApi/Bookmarks/MWMBookmarksManager.mm` | Remove unused `SetBookmarksChangedCallback` setup (if any), implement observer |
| `android/sdk/src/main/cpp/.../BookmarkManager.cpp` | Migrate from callback to observer pattern |

## Tests

### Existing (must pass)
- All tests in `libs/map/map_tests/bookmarks_test.cpp`

### New tests
```cpp
// Test: newly created category triggers save
TEST(BookmarkManager, CreatedCategoryIsSaved)
{
  auto catId = bmManager.CreateBookmarkCategory("NewCat");
  // Verify file exists on disk
  auto const fileName = bmManager.GetCategoryFileName(catId);
  TEST(!fileName.empty(), ());
  TEST(Platform::IsFileExistsByFullPath(fileName), ());
}

// Test: batch file loading processes all files in single pass
TEST(BookmarkManager, BatchLoadBookmarks)
{
  // Create 3 KML files
  // Call LoadBookmarks({file1, file2, file3}, false);
  // Verify all 3 categories created
  // Verify onFinished called exactly once
}

// Test: observer receives events for created categories
TEST(BookmarkManager, ObserverReceivesCreatedCategoryEvent)
{
  TestObserver observer;
  bmManager.AddObserver(observer);
  bmManager.CreateBookmarkCategory("Test");
  TEST(observer.m_categoriesChanged, ());
  bmManager.RemoveObserver(observer);
}
```

## Risks & Mitigations

- **`HasBookmarksChanges()` fix may change save behavior**: The intentional exclusion of created groups (for `LoadBookmark`/`ReloadBookmark`) needs to be preserved. Mitigation: `LoadBookmark`/`ReloadBookmark` already call `CreateCategories()` with `autoSave` parameter, which handles persistence directly. Adding created groups to the auto-save path in `NotifyChanges()` should be safe because `CreateCategories()` calls `NotifyChanges(!isUpdating)` where `isUpdating=false` for new categories.
- **Batch loading API change**: Callers currently pass one file at a time. Mitigation: keep single-file overload that calls batch version with single-element vector.
- **Observer pattern migration**: Both iOS and Android need updating. Mitigation: keep old callbacks as deprecated during transition, remove after both platforms migrated.

## Priority
Phase 2, but Bug fixes (Step 0) should be Phase 1 alongside Plan H dead code cleanup.
