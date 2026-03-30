# C. Extract Drape Communication

## Problem

BookmarkManager directly communicates with the rendering engine (Drape), coupling data management to rendering:

1. **Owns** `m_drapeEngine` (`DrapeEngineSafePtr`) -- line 731 of `bookmark_manager.hpp`
2. **Owns** `m_drapeChangesTracker` (`MarksChangesTracker`) -- line 730
3. **`NotifyChanges()`** (lines 623-641 of .cpp) locks Drape engine, updates visibility, pushes mark changes, invalidates
4. **`RequestSymbolSizes()`** (lines 1822-1843) makes async Drape call, handles response on GUI thread
5. **`UpdateBookmarksTextPlacement()`** (lines 644-649) calls Drape directly
6. **`SetDrapeEngine()`** (lines 1845-1855) lifecycle method triggers full sync + symbol request
7. **`MarksChangesTracker`** implements `df::UserMarksProvider` (Drape rendering interface)

Impact: BookmarkManager can't be tested without a Drape engine (test mode null-checks it). Data management is entangled with rendering concerns. The `DrapeEngineSafePtr` mutex is held during mark updates, potentially blocking rendering.

## Proposed Solution

Create `BookmarkRenderAdapter` that subscribes to BookmarkManager changes and pushes them to Drape:

```cpp
// libs/map/bookmark_render_adapter.hpp
class BookmarkRenderAdapter
{
public:
  explicit BookmarkRenderAdapter(BookmarkManager & bmManager);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  // Called by BookmarkManager::NotifyChanges() with accumulated changes
  void OnChanges(MarksChangesTracker const & changes, bool firstNotification);

  void UpdateBookmarksTextPlacement();

  bool AreSymbolSizesAcquired() const { return m_symbolSizesAcquired; }
  void SetOnSymbolSizesAcquired(std::function<void()> && callback);
  m2::PointF GetMaxBookmarkSymbolSize() const { return m_maxBookmarkSymbolSize; }

private:
  void RequestSymbolSizes();
  void PushChangesToDrape(bool firstNotification);

  BookmarkManager & m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;
  MarksChangesTracker m_drapeChangesTracker;
  bool m_firstNotification = false;
  m2::PointF m_maxBookmarkSymbolSize;
  bool m_symbolSizesAcquired = false;
  std::function<void()> m_onSymbolSizesAcquiredFn;
};
```

### Modified NotifyChanges()

```cpp
void BookmarkManager::NotifyChanges(bool saveChangesOnDisk)
{
  // ... existing accumulation and persistence logic ...

  // Instead of direct Drape calls:
  if (m_renderAdapter)
    m_renderAdapter->OnChanges(m_changesTracker, m_firstDrapeNotification);

  m_firstDrapeNotification = false;
  m_changesTracker.ResetChanges();
}
```

### BookmarkRenderAdapter::OnChanges()

```cpp
void BookmarkRenderAdapter::OnChanges(MarksChangesTracker const & changes, bool firstNotification)
{
  m_drapeChangesTracker.AddChanges(changes);

  if (!m_drapeChangesTracker.HasChanges() && !firstNotification)
    return;

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    auto engine = lock.Get();
    for (auto groupId : m_drapeChangesTracker.GetUpdatedGroupIds())
      engine->ChangeVisibilityUserMarksGroup(groupId, m_bmManager.GetGroup(groupId)->IsVisible());
    for (auto groupId : m_drapeChangesTracker.GetRemovedGroupIds())
      engine->ClearUserMarksGroup(groupId);
    engine->UpdateUserMarks(&m_drapeChangesTracker, firstNotification);
    engine->InvalidateUserMarks();
  }
  m_drapeChangesTracker.ResetChanges();
}
```

## Files to Create

| File | Content |
|------|---------|
| `libs/map/bookmark_render_adapter.hpp` | Class declaration |
| `libs/map/bookmark_render_adapter.cpp` | ~100 lines: OnChanges, RequestSymbolSizes, SetDrapeEngine |

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove: `m_drapeEngine` (line 731), `m_drapeChangesTracker` (line 730), `m_maxBookmarkSymbolSize` (line 772), `m_symbolSizesAcquired` (line 743), `m_onSymbolSizesAcquiredFn` (line 742), `m_firstDrapeNotification` (line 750). Add: `m_renderAdapter` pointer. Remove: `SetDrapeEngine()`, `RequestSymbolSizes()`, `UpdateBookmarksTextPlacement()`, `AreSymbolSizesAcquired()`. |
| `libs/map/bookmark_manager.cpp` | Remove Drape-related code from `NotifyChanges()` (lines 623-641). Remove `SetDrapeEngine()` (lines 1845-1855), `RequestSymbolSizes()` (lines 1822-1843), `UpdateBookmarksTextPlacement()` (lines 644-649). |
| `libs/map/framework.hpp` | Own `BookmarkRenderAdapter` as member. |
| `libs/map/framework.cpp` | Create adapter, wire `SetDrapeEngine()` to adapter instead of BookmarkManager. |
| `libs/map/CMakeLists.txt` | Add new source files. |

## Tests

### Existing (must pass)
- All `bookmarks_test.cpp` tests use test mode which skips Drape -- they will pass without change.

### New tests
```cpp
// Test: BookmarkRenderAdapter receives changes
TEST(BookmarkRenderAdapter, ReceivesChangesFromTracker)
{
  MockDrapeEngine mockEngine;
  BookmarkRenderAdapter adapter(bmManager);
  adapter.SetDrapeEngine(ref_ptr<df::DrapeEngine>(&mockEngine));

  MarksChangesTracker tracker(&bmManager);
  tracker.OnAddMark(markId);
  tracker.OnAddGroup(groupId);
  tracker.AcceptDirtyItems();

  adapter.OnChanges(tracker, false);

  TEST(mockEngine.UpdateUserMarksCalled(), ());
  TEST(mockEngine.InvalidateUserMarksCalled(), ());
}

// Test: No Drape calls when engine not set
TEST(BookmarkRenderAdapter, NoCrashWithoutEngine)
{
  BookmarkRenderAdapter adapter(bmManager);
  MarksChangesTracker tracker(&bmManager);
  adapter.OnChanges(tracker, false);  // Should not crash
}

// Test: First notification triggers full sync
TEST(BookmarkRenderAdapter, FirstNotificationFullSync)
{
  MockDrapeEngine mockEngine;
  BookmarkRenderAdapter adapter(bmManager);
  adapter.SetDrapeEngine(ref_ptr<df::DrapeEngine>(&mockEngine));

  MarksChangesTracker emptyTracker(&bmManager);
  adapter.OnChanges(emptyTracker, true);  // first=true

  TEST(mockEngine.UpdateUserMarksCalled(), ());
  TEST(mockEngine.LastUpdateWasFirstNotification(), ());
}
```

## Risks & Mitigations

- **Notification ordering**: Drape update must happen after persistence callbacks (platform may query data that was just saved). Current code does this correctly; the adapter preserves the order since `OnChanges` is called at the end of `NotifyChanges`.
- **`m_firstDrapeNotification`**: Set to true in `SetDrapeEngine()`, consumed in `NotifyChanges()`. Moves to adapter. Mitigation: adapter's `SetDrapeEngine()` sets `m_firstNotification = true` and triggers initial sync.
- **Symbol sizes callback chain**: `RequestSymbolSizes()` uses `m_trackInfoMarkId` which stays in BookmarkManager. Adapter needs to call back into BookmarkManager to update the mark. Mitigation: adapter takes a `std::function<void(m2::PointF, m2::PointF)>` callback for symbol size results.

## Dependencies
- Strongly benefits from plan B (unify change notification) -- reduces from 3 trackers to 2, making extraction cleaner.
- Can be done independently with 2 remaining trackers, but the code will be slightly more complex.

## Priority
Phase 3 (after plan B).
