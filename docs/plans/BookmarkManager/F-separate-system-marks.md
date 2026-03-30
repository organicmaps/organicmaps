# F. Separate System Marks

## Problem

BookmarkManager manages both **user-created bookmarks** (persistent, file-backed) and **ephemeral system marks** (app-owned, never saved) through the same data structures and change tracking:

### System marks in BookmarkManager:
- `m_selectionMark` (`StaticMarkPoint *`) -- selection indicator, created in constructor (line 767)
- `m_myPositionMark` (`MyPositionMarkPoint *`) -- my position, created in constructor (line 768)
- `m_trackInfoMarkId` (`kml::MarkId`) -- track info popup, single instance (line 770)
- `TrackSelectionMark` -- elevation profile cursor, dynamically created/destroyed per track
- Search marks (`UserMark::Type::SEARCH`)
- Route marks (`UserMark::Type::ROUTING`)
- Speed camera marks, transit marks, local ads marks, debug marks, etc.

### Storage:
- `m_userMarks` (`map<MarkId, unique_ptr<UserMark>>`) -- all non-bookmark marks (line 763)
- `m_userMarkLayers` (`vector<unique_ptr<UserMarkLayer>>`) -- layers by type (line 761)

### Impact:
- `MarksChangesTracker` tracks changes to both bookmarks and system marks
- ID system, visibility logic, dirty flags all shared between both
- `FindNearestUserMark()` searches across all mark types
- `CreateUserMark<>()` template handles both bookmark-related marks and system marks
- BookmarkManager's responsibilities are inflated by ~200 lines of system mark code

## Proposed Solution

Create `SystemMarkManager` for non-bookmark marks:

```cpp
// libs/map/system_mark_manager.hpp

class SystemMarkManager
{
public:
  SystemMarkManager();

  // Special marks
  StaticMarkPoint & SelectionMark() { return *m_selectionMark; }
  StaticMarkPoint const & SelectionMark() const { return *m_selectionMark; }
  MyPositionMarkPoint & MyPositionMark() { return *m_myPositionMark; }
  MyPositionMarkPoint const & MyPositionMark() const { return *m_myPositionMark; }

  // Generic mark CRUD
  template <typename UserMarkT>
  UserMarkT * CreateUserMark(m2::PointD const & ptOrg);

  template <typename UserMarkT>
  UserMarkT const * GetMark(kml::MarkId markId) const;

  template <typename UserMarkT>
  UserMarkT * GetMarkForEdit(kml::MarkId markId);

  UserMark const * GetUserMark(kml::MarkId markId) const;
  UserMark * GetUserMarkForEdit(kml::MarkId markId);

  void DeleteUserMark(kml::MarkId markId);

  template <typename UserMarkT, typename F>
  void DeleteUserMarks(UserMark::Type type, F && deletePredicate);

  kml::MarkIdSet const & GetUserMarkIds(UserMark::Type type) const;

  // Change tracking
  BookmarkManager::MarksChangesTracker & GetChangesTracker() { return m_changesTracker; }

  // Editing session support
  EditSession GetEditSession();

private:
  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkLayer>>;
  using MarksCollection = std::map<kml::MarkId, std::unique_ptr<UserMark>>;

  UserMarkLayers m_userMarkLayers;
  MarksCollection m_userMarks;
  StaticMarkPoint * m_selectionMark = nullptr;
  MyPositionMarkPoint * m_myPositionMark = nullptr;

  BookmarkManager::MarksChangesTracker m_changesTracker;
  ThreadChecker m_threadChecker;
};
```

### What Moves from BookmarkManager:

**Members removed:**
- `m_userMarks` (line 763) -- except bookmark entries
- `m_userMarkLayers` (line 761) -- all layers except BOOKMARK type
- `m_selectionMark` (line 767)
- `m_myPositionMark` (line 768)

**Methods removed:**
- `SelectionMark()` / `MyPositionMark()` accessors (lines 311-315)
- Template `CreateUserMark<>()` for non-bookmark types (lines 524-539)
- Template `GetMarkForEdit<>()` for non-bookmark types (lines 541-548)
- Template `DeleteUserMarks<>()` (lines 550-561)
- `GetMark<>()` template (lines 205-211) -- split between both managers

**Methods that need access to both:**
- `FindNearestUserMark()` (lines 296-300) -- searches all mark types
- `UpdateViewport()` -- may update both

### What Stays in BookmarkManager:
- `m_bookmarks` (BookmarksCollection)
- `m_tracks` (TracksCollection)
- `m_categories` (CategoriesCollection)
- Bookmark-specific CreateBookmark/DeleteBookmark/etc.
- `m_compilations`

## Shared Change Tracking

The challenge: both managers' changes must reach Drape as a single update. Options:

**Option A: Shared tracker** (recommended)
Both managers share a `MarksChangesTracker` owned by Framework:
```cpp
class Framework {
  MarksChangesTracker m_marksChangesTracker;
  SystemMarkManager m_systemMarkManager{m_marksChangesTracker};
  BookmarkManager m_bookmarkManager{m_marksChangesTracker, ...};
};
```

**Option B: Merged updates**
Framework merges changes from both before pushing to Drape:
```cpp
void Framework::NotifyDrape() {
  m_drapeTracker.AddChanges(m_bmManager.GetChanges());
  m_drapeTracker.AddChanges(m_sysMarkManager.GetChanges());
  // push to Drape
}
```

Option A is simpler and preserves the current Drape update mechanism.

## Files to Create

| File | Content |
|------|---------|
| `libs/map/system_mark_manager.hpp` | SystemMarkManager class |
| `libs/map/system_mark_manager.cpp` | Implementation (~200 lines) |

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove system mark members, templates, accessors |
| `libs/map/bookmark_manager.cpp` | Remove system mark code |
| `libs/map/framework.hpp` | Own SystemMarkManager, expose SelectionMark/MyPositionMark |
| `libs/map/framework.cpp` | Create SystemMarkManager, wire change tracking |
| `libs/map/CMakeLists.txt` | Add new files |

### Callers that use system marks (need updating):
```
grep -rn "SelectionMark\|MyPositionMark\|CreateUserMark<Search\|CreateUserMark<Route\|CreateUserMark<SpeedCam" libs/ --include="*.cpp"
```

These callers need to use `framework.GetSystemMarkManager()` instead of `framework.GetBookmarkManager()`.

## Tests

### Existing (must pass)
- `bookmarks_test.cpp` -- bookmark operations should be unaffected

### New tests
```cpp
// libs/map/map_tests/system_mark_manager_test.cpp

TEST(SystemMarkManager, CreateAndGetMark)
{
  SystemMarkManager mgr;
  auto * mark = mgr.CreateUserMark<SearchMarkPoint>(m2::PointD(10, 20));
  TEST(mark != nullptr, ());
  auto const * retrieved = mgr.GetMark<SearchMarkPoint>(mark->GetId());
  TEST_EQUAL(retrieved, mark, ());
}

TEST(SystemMarkManager, DeleteMark)
{
  SystemMarkManager mgr;
  auto * mark = mgr.CreateUserMark<SearchMarkPoint>(m2::PointD(10, 20));
  auto const id = mark->GetId();
  mgr.DeleteUserMark(id);
  TEST(mgr.GetUserMark(id) == nullptr, ());
}

TEST(SystemMarkManager, SelectionMarkExists)
{
  SystemMarkManager mgr;
  auto & sel = mgr.SelectionMark();
  sel.SetPtOrg(m2::PointD(5, 5));
  TEST_EQUAL(mgr.SelectionMark().GetPivot(), m2::PointD(5, 5), ());
}

TEST(SystemMarkManager, MyPositionMark)
{
  SystemMarkManager mgr;
  mgr.MyPositionMark().SetUserPosition(m2::PointD(1, 2), true);
  TEST(mgr.MyPositionMark().HasPosition(), ());
}

TEST(SystemMarkManager, DeleteByPredicate)
{
  SystemMarkManager mgr;
  mgr.CreateUserMark<SearchMarkPoint>(m2::PointD(1, 1));
  mgr.CreateUserMark<SearchMarkPoint>(m2::PointD(2, 2));
  mgr.CreateUserMark<SearchMarkPoint>(m2::PointD(3, 3));

  mgr.DeleteUserMarks<SearchMarkPoint>(UserMark::SEARCH,
    [](SearchMarkPoint const * mark) { return mark->GetPivot().x > 1.5; });

  TEST_EQUAL(mgr.GetUserMarkIds(UserMark::SEARCH).size(), 1, ());
}
```

## Risks & Mitigations

- **`FindNearestUserMark()`**: Searches across all mark types including bookmarks. Must query both managers. Mitigation: Framework implements the combined search.
- **Change tracker coupling**: Both managers need their changes tracked for Drape. Mitigation: shared tracker (Option A).
- **ID space**: System marks and bookmarks share the ID space (MarkType encoded in upper bits). No conflict as long as both managers use the same `UserMarkIdStorage`. Mitigation: `UserMarkIdStorage` is a singleton, safe to share.
- **Track selection marks**: `TrackSelectionMark` and `TrackInfoMark` are system marks but managed by TrackSelectionManager (plan A). They live in SystemMarkManager but are accessed via TrackSelectionManager. This works because TrackSelectionManager references SystemMarkManager.

## Dependencies
- Benefits from plan A (split components) -- TrackSelectionManager separates track marks cleanly
- Independent of plans B, C, D

## Priority
Phase 3. Medium effort, medium impact.
