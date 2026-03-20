# B. Unify Change Notification

## Problem

BookmarkManager has 12+ callback mechanisms across 3 layers and 3 parallel change trackers.

### Current Callback Inventory

**Layer 1 -- Constructor callbacks** (permanent, set once in Framework):
- `m_createdBookmarksCallback` -- `std::function<void(vector<BookmarkInfo> const &)>`
- `m_updatedBookmarksCallback` -- same signature
- `m_deletedBookmarksCallback` -- `std::function<void(vector<kml::MarkId> const &)>`
- `m_attachedBookmarksCallback` -- `std::function<void(vector<BookmarkGroupInfo> const &)>`
- `m_detachedBookmarksCallback` -- same signature

**Layer 2 -- Settable callbacks** (set by platform bridges):
- `m_bookmarksChangedCallback` -- `std::function<void()>`
- `m_categoriesChangedCallback` -- `std::function<void()>`
- `m_elevationActivePointChanged` -- `std::function<void()>`
- `m_elevationMyPositionChanged` -- `std::function<void()>`
- `m_onSymbolSizesAcquiredFn` -- `std::function<void()>`

**Layer 3 -- Async loading callbacks** (set by platform bridges):
- `m_asyncLoadingCallbacks.m_onStarted`
- `m_asyncLoadingCallbacks.m_onFinished`
- `m_asyncLoadingCallbacks.m_onFileSuccess`
- `m_asyncLoadingCallbacks.m_onFileError`

### Current Tracker Flow

Three `MarksChangesTracker` instances (lines 728-730 of `bookmark_manager.hpp`):

```
m_changesTracker           -- accumulates during edit sessions
m_bookmarksChangesTracker  -- merged, drives persistence + platform callbacks
m_drapeChangesTracker      -- merged, drives rendering updates
```

`NotifyChanges()` flow (lines 580-642 of `bookmark_manager.cpp`):
```
1. m_changesTracker.AcceptDirtyItems()
2. if HasBookmarksChanges -> NotifyBookmarksChanged() (simple callback)
3. if HasCategoriesChanges -> NotifyCategoriesChanged() (simple callback)
4. m_bookmarksChangesTracker.AddChanges(m_changesTracker)
5. m_drapeChangesTracker.AddChanges(m_changesTracker)
6. m_changesTracker.ResetChanges()
7. if bookmarks changed:
   a. Collect dirty categories -> SaveBookmarks()
   b. SendBookmarksChanges() -> 5 constructor callbacks
8. m_bookmarksChangesTracker.ResetChanges()
9. if drape changes:
   a. Lock DrapeEngine
   b. Update visibility, push marks, invalidate
10. m_drapeChangesTracker.ResetChanges()
```

The key observation: steps 4 and 5 are identical -- both trackers receive the same `AddChanges()`. They exist only because they're consumed (and reset) at different points.

## Proposed Solution

### Step 1: Eliminate one tracker

Replace 3 trackers with 2:
- `m_changesTracker` -- accumulation during edit session (keep as-is)
- Eliminate `m_bookmarksChangesTracker` by processing persistence and platform callbacks directly from `m_changesTracker` before drape update

```cpp
void BookmarkManager::NotifyChanges(bool saveChangesOnDisk)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_changesTracker.AcceptDirtyItems();

  if (!m_firstDrapeNotification && !m_changesTracker.HasChanges()
      && !m_drapeChangesTracker.HasChanges())
    return;

  // 1. Simple callbacks (from m_changesTracker)
  if (m_changesTracker.HasBookmarksChanges())
    NotifyBookmarksChanged();
  if (m_changesTracker.HasCategoriesChanges())
    NotifyCategoriesChanged();

  // 2. Persistence + detailed callbacks (from m_changesTracker)
  if (!m_notificationsEnabled)
  {
    // Still need to accumulate for drape
    m_drapeChangesTracker.AddChanges(m_changesTracker);
    m_changesTracker.ResetChanges();
    return;
  }

  if (m_changesTracker.HasBookmarksChanges())
  {
    // Auto-save
    if (saveChangesOnDisk)
    {
      kml::GroupIdCollection categoriesToSave;
      for (auto groupId : m_changesTracker.GetUpdatedGroupIds())
        if (IsBookmarkCategory(groupId) && GetBmCategory(groupId)->IsAutoSaveEnabled())
          categoriesToSave.push_back(groupId);
      SaveBookmarks(categoriesToSave);
    }

    // Platform callbacks
    SendBookmarksChanges(m_changesTracker);
  }

  // 3. Drape update
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

This eliminates `m_bookmarksChangesTracker` entirely.

### Step 2: Unified observer interface (future)

Replace constructor callbacks + settable callbacks:

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

// In BookmarkManager:
void AddObserver(BookmarkObserver & observer);
void RemoveObserver(BookmarkObserver & observer);
```

Framework implements BookmarkObserver for search indexing. Platform bridges implement for UI updates.

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove `m_bookmarksChangesTracker` (line 729). Add `BookmarkObserver` interface. Remove `Callbacks` struct members for created/updated/deleted/attached/detached. |
| `libs/map/bookmark_manager.cpp` | Rewrite `NotifyChanges()` (lines 580-642). Update `SendBookmarksChanges()` to use observer list. Remove `AddChanges` to bookmarks tracker. |
| `libs/map/framework.cpp` | Implement `BookmarkObserver` instead of passing 5 callbacks in `Callbacks` constructor. |

## Tests

### Existing (must pass)
- All tests in `libs/map/map_tests/bookmarks_test.cpp`

### New tests
```cpp
// Test: observer receives OnBookmarksCreated when bookmark is added
TEST(BookmarkManager, ObserverReceivesCreatedCallback)
{
  TestBookmarkObserver observer;
  bmManager.AddObserver(observer);
  {
    auto session = bmManager.GetEditSession();
    session.CreateBookmark(MakeBookmarkData(), categoryId);
  }
  TEST(!observer.m_createdBookmarks.empty(), ());
  bmManager.RemoveObserver(observer);
}

// Test: nested sessions batch notifications
TEST(BookmarkManager, NestedSessionsBatchNotifications)
{
  CountingObserver observer;
  bmManager.AddObserver(observer);
  {
    auto session1 = bmManager.GetEditSession();
    session1.CreateBookmark(data1, catId);
    {
      auto session2 = bmManager.GetEditSession();
      session2.CreateBookmark(data2, catId);
    }
    // session2 closed, but session1 still open -- no notification yet
    TEST_EQUAL(observer.m_callCount, 0, ());
  }
  // session1 closed -- single notification with both bookmarks
  TEST_EQUAL(observer.m_callCount, 1, ());
  TEST_EQUAL(observer.m_lastCreatedCount, 2, ());
  bmManager.RemoveObserver(observer);
}

// Test: m_notificationsEnabled=false suppresses platform callbacks but still updates drape
TEST(BookmarkManager, NotificationsDisabledStillUpdatesDrape)
{
  // ...
}
```

## Risks & Mitigations

- **Notification ordering change**: Current code fires simple callbacks *before* accumulating into bookmarks/drape trackers. New code fires them before persistence+drape. Same relative order. Mitigation: verify with existing tests.
- **`m_notificationsEnabled` semantics**: Currently suppresses both persistence and drape. Must preserve: when disabled, changes still accumulate in drape tracker for later flush. The proposed code handles this correctly.
- **`m_firstDrapeNotification`**: Forces full sync when drape engine first connects. Preserved in new flow.

## Priority
Phase 2. Can be done before or after plan A (split components). Pairs well with plan C (extract drape).
