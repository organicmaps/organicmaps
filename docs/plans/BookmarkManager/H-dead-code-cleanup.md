# H. Dead Code Cleanup

## Problem

Several unused artifacts remain in BookmarkManager, likely from removed cloud sync features.

## Items

### 1. `ExpiredCategory` struct -- CONFIRMED DEAD

**Definition** (`libs/map/bookmark_manager.hpp`, lines 798-805):
```cpp
struct ExpiredCategory
{
  ExpiredCategory(kml::MarkGroupId id, std::string const & serverId) : m_id(id), m_serverId(serverId) {}
  kml::MarkGroupId m_id;
  std::string m_serverId;
};
std::vector<ExpiredCategory> m_expiredCategories;
```

**Evidence**: Grep for `ExpiredCategory` and `m_expiredCategories` in `bookmark_manager.cpp` returns zero results. The struct and vector are declared but never used anywhere in the codebase.

**Action**: Remove struct definition and member variable.

### 2. `GetSeacrhAPIFn` typo -- CONFIRMED TYPO

**Definition** (`libs/map/bookmark_manager.hpp`, line 73):
```cpp
using GetSeacrhAPIFn = std::function<SearchAPI &()>;
```

**Member** (line 95):
```cpp
GetSeacrhAPIFn m_getSearchAPI;
```

The typo is "Seacrh" instead of "Search". Additionally, `m_getSearchAPI` is **declared but never dereferenced** in `bookmark_manager.cpp`. It's only used in tests where it's passed as `nullptr`:

```cpp
// libs/map/map_tests/bookmarks_test.cpp, line 143 and 1241
static_cast<BookmarkManager::Callbacks::GetSeacrhAPIFn>(nullptr)
```

**Action**:
- Rename `GetSeacrhAPIFn` to `GetSearchAPIFn`
- Update the test file reference
- Consider removing `m_getSearchAPI` entirely if truly unused, or keep for future use

### 3. Confirmed STILL IN USE -- DO NOT REMOVE

**`RestoringCache` / `m_restoringCache`**:
Used in `CreateCategories()` (bookmark_manager.cpp, line 2754-2760) to restore access rules and server IDs during category loading. Active code path.

**`m_lastCategoryUrl`**:
Used for persisting last edited category to settings:
- Line 1922: `settings::Set(kLastEditedBookmarkCategory, m_lastCategoryUrl);`
- Line 1928: `settings::TryGet(kLastEditedBookmarkCategory, m_lastCategoryUrl);`
- Line 2312: `if (cat.second->GetFileName() == m_lastCategoryUrl)`
- Line 2331: `m_lastCategoryUrl = GetBmCategory(groupId)->GetFileName();`

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
// libs/map/bookmark_manager.hpp, line 73
- using GetSeacrhAPIFn = std::function<SearchAPI &()>;
+ using GetSearchAPIFn = std::function<SearchAPI &()>;
```

```diff
// libs/map/map_tests/bookmarks_test.cpp, lines 143 and 1241
- static_cast<BookmarkManager::Callbacks::GetSeacrhAPIFn>(nullptr)
+ static_cast<BookmarkManager::Callbacks::GetSearchAPIFn>(nullptr)
```

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove `ExpiredCategory` struct + `m_expiredCategories` (lines 798-805). Rename `GetSeacrhAPIFn` -> `GetSearchAPIFn` (line 73). |
| `libs/map/map_tests/bookmarks_test.cpp` | Update cast references (lines 143, 1241). |

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
# Update all occurrences

# Build and run tests
cmake --build build --target map_tests
./build/map_tests
```

## Risks
- **Very low**: Removing proven-unused code and fixing a typo.

## Priority
Phase 1 (quick win). ~30 minutes of work.
