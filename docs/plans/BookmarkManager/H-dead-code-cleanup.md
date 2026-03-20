# H. Dead Code Cleanup

## Problem

Several unused artifacts remain in BookmarkManager, from removed cloud sync features and accumulated technical debt.

## Items

### 1. `ExpiredCategory` struct -- CONFIRMED DEAD

**Definition** (`libs/map/bookmark_manager.hpp`):
```cpp
struct ExpiredCategory
{
  ExpiredCategory(kml::MarkGroupId id, std::string const & serverId) : m_id(id), m_serverId(serverId) {}
  kml::MarkGroupId m_id;
  std::string m_serverId;
};
std::vector<ExpiredCategory> m_expiredCategories;
```

**Evidence**: Grep for `ExpiredCategory` and `m_expiredCategories` in `bookmark_manager.cpp` returns zero results.

**Action**: Remove struct definition and member variable.

### 2. `GetSeacrhAPIFn` typo -- CONFIRMED TYPO

**Definition** (`libs/map/bookmark_manager.hpp`):
```cpp
using GetSeacrhAPIFn = std::function<SearchAPI &()>;
```

The typo is "Seacrh" instead of "Search". `m_getSearchAPI` is declared but **never dereferenced** in `bookmark_manager.cpp`. Only used in tests as `nullptr`.

**Action**: Rename `GetSeacrhAPIFn` to `GetSearchAPIFn`. Update test references.

### 3. `HasCategoriesChanges()` -- NEARLY UNUSED, INCOMPLETE

**Definition** (`libs/map/bookmark_manager.cpp`):
```cpp
bool MarksChangesTracker::HasCategoriesChanges() const
{
  return HasBookmarkCategories(m_createdGroups) || HasBookmarkCategories(m_removedGroups);
  // Missing: m_updatedGroups (e.g. category renames)
}
```

**Usage**: Only called from `NotifyChanges()` to gate `NotifyCategoriesChanged()`. The gated callback (`m_categoriesChangedCallback`) is not set on iOS, making this effectively dead on that platform.

**Action**: Remove as part of Plan B (change notification unification). If removed independently, replace the call in `NotifyChanges()` with a direct `HasChanges()` check, or always call `NotifyCategoriesChanged()` when there are group changes.

### 4. `m_bookmarksChangedCallback` / `SetBookmarksChangedCallback` -- NOT USED ON iOS

**Definition** (`libs/map/bookmark_manager.hpp`):
```cpp
using BookmarksChangedCallback = std::function<void()>;
void SetBookmarksChangedCallback(BookmarksChangedCallback && callback);
```

Set on Android (JNI `OnBookmarksChanged()`) but NOT set on iOS. `NotifyBookmarksChanged()` is effectively a no-op on iOS.

**Action**: Remove as part of Plan B (replace with unified observer pattern). If removed independently, also remove the Android callback registration and have Android use the same mechanism as iOS.

### 5. Confirmed STILL IN USE -- DO NOT REMOVE

**`RestoringCache` / `m_restoringCache`**:
Used in `CreateCategories()` to restore access rules and server IDs during category loading. Active code path.

**`m_lastCategoryUrl`**:
Used for persisting last edited category to settings.

## Implementation

### Step 1: Remove `ExpiredCategory`

```diff
// libs/map/bookmark_manager.hpp
- struct ExpiredCategory
- {
-   ExpiredCategory(kml::MarkGroupId id, std::string const & serverId) : m_id(id), m_serverId(serverId) {}
-
-   kml::MarkGroupId m_id;
-   std::string m_serverId;
- };
- std::vector<ExpiredCategory> m_expiredCategories;
```

### Step 2: Fix typo

```diff
// libs/map/bookmark_manager.hpp
- using GetSeacrhAPIFn = std::function<SearchAPI &()>;
+ using GetSearchAPIFn = std::function<SearchAPI &()>;
```

```diff
// libs/map/map_tests/bookmarks_test.cpp
- static_cast<BookmarkManager::Callbacks::GetSeacrhAPIFn>(nullptr)
+ static_cast<BookmarkManager::Callbacks::GetSearchAPIFn>(nullptr)
```

### Step 3: Remove unused callbacks (optional, can defer to Plan B)

```diff
// libs/map/bookmark_manager.hpp
- using BookmarksChangedCallback = std::function<void()>;
- using CategoriesChangedCallback = std::function<void()>;
- void SetBookmarksChangedCallback(BookmarksChangedCallback && callback);
- void SetCategoriesChangedCallback(CategoriesChangedCallback && callback);

// libs/map/bookmark_manager.cpp
- BookmarksChangedCallback m_bookmarksChangedCallback;
- CategoriesChangedCallback m_categoriesChangedCallback;
- void BookmarkManager::NotifyBookmarksChanged() { ... }
- void BookmarkManager::NotifyCategoriesChanged() { ... }
```

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove `ExpiredCategory` + `m_expiredCategories`. Rename `GetSeacrhAPIFn` -> `GetSearchAPIFn`. Optionally remove unused callback types. |
| `libs/map/bookmark_manager.cpp` | Remove any `ExpiredCategory` references. Optionally remove `NotifyBookmarksChanged()`/`NotifyCategoriesChanged()`. |
| `libs/map/map_tests/bookmarks_test.cpp` | Update `GetSeacrhAPIFn` cast references. |
| `android/sdk/src/main/cpp/.../BookmarkManager.cpp` | If removing callbacks: remove `OnBookmarksChanged()` JNI callback. |

## Tests

- All existing tests must compile and pass
- No new tests needed -- this is dead code removal and a rename

## Verification

```bash
# Verify ExpiredCategory is unused
grep -rn "ExpiredCategory\|m_expiredCategories" libs/map/bookmark_manager.cpp
# Expected: no results

# Verify typo fix compiles
grep -rn "GetSeacrhAPIFn\|GetSearchAPIFn" libs/ --include="*.hpp" --include="*.cpp"

# Build and run tests
cmake --build build --target map_tests
./build/map_tests
```

## Risks
- **Very low**: Removing proven-unused code and fixing a typo.
- **Low** (callback removal): Android `OnBookmarksChanged()` would need to be removed or redirected. Safer to defer to Plan B.

## Priority
Phase 1 (quick win). Steps 1-2 are ~30 minutes of work. Step 3 can be deferred to Plan B.
