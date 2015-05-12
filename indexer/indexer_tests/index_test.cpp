#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/index.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/string.hpp"

void CheckedDeleteFile(string const & file)
{
  if (Platform::IsFileExistsByFullPath(file))
    CHECK(my::DeleteFileX(file), ("Can't remove file:", file));
}

class Observer : public Index::Observer
{
public:
  Observer(string const & file)
      : m_file(file),
        m_map_registered_calls(0),
        m_map_update_is_ready_calls(0),
        m_map_updated_calls(0),
        m_map_deleted_calls(0)
  {
  }

  // Index::Observer overrides:
  void OnMapRegistered(string const & file) override
  {
    CHECK_EQUAL(m_file, file, ());
    ++m_map_registered_calls;
  }

  void OnMapUpdateIsReady(string const & file) override
  {
    CHECK_EQUAL(m_file, file, ());
    ++m_map_update_is_ready_calls;
  }

  void OnMapUpdated(string const & file) override
  {
    CHECK_EQUAL(m_file, file, ());
    ++m_map_updated_calls;
  }

  void OnMapDeleted(string const & file) override
  {
    CHECK_EQUAL(m_file, file, ());
    ++m_map_deleted_calls;
  }

  int map_registered_calls() const { return m_map_registered_calls; }
  int map_update_is_ready_calls() const { return m_map_update_is_ready_calls; }
  int map_updated_calls() const { return m_map_updated_calls; }
  int map_deleted_calls() const { return m_map_deleted_calls; }

private:
  string const m_file;
  int m_map_registered_calls;
  int m_map_update_is_ready_calls;
  int m_map_updated_calls;
  int m_map_deleted_calls;
};

UNIT_TEST(Index_Parse)
{
  Index index;
  UNUSED_VALUE(index.RegisterMap("minsk-pass" DATA_FILE_EXTENSION));

  // Make sure that index is actually parsed.
  NoopFunctor fn;
  index.ForEachInScale(fn, 15);
}

UNIT_TEST(Index_MwmStatusNotifications)
{
  string const resourcesDir = GetPlatform().ResourcesDir();
  string const sourceMapName = "minsk-pass" DATA_FILE_EXTENSION;
  string const sourceMapPath = my::JoinFoldersToPath(resourcesDir, sourceMapName);
  string const testMapName = "minsk-pass-copy" DATA_FILE_EXTENSION;
  string const testMapPath = my::JoinFoldersToPath(resourcesDir, testMapName);
  string const testMapUpdatePath = testMapPath + READY_FILE_EXTENSION;

  TEST(my::CopyFileX(sourceMapPath, testMapPath), ());
  MY_SCOPE_GUARD(testMapGuard, bind(&CheckedDeleteFile, testMapPath));

  Index index;
  Observer observer(testMapName);
  index.AddObserver(observer);

  TEST_EQUAL(0, observer.map_registered_calls(), ());

  // Checks that observers are triggered after map registration.
  {
    pair<MwmSet::MwmLock, bool> const p = index.RegisterMap(testMapName);
    TEST(p.first.IsLocked(), ());
    TEST(p.second, ());
    TEST_EQUAL(1, observer.map_registered_calls(), ());
  }

  // Checks that map can't registered twice and observers aren't
  // triggered.
  {
    pair<MwmSet::MwmLock, bool> const p = index.RegisterMap(testMapName);
    TEST(p.first.IsLocked(), ());
    TEST(!p.second, ());
    TEST_EQUAL(1, observer.map_registered_calls(), ());
  }

  TEST(my::CopyFileX(testMapPath, testMapUpdatePath), ());
  MY_SCOPE_GUARD(testMapUpdateGuard, bind(&CheckedDeleteFile, testMapUpdatePath));

  // Checks that observers are notified when map is deleted.
  {
    TEST_EQUAL(0, observer.map_update_is_ready_calls(), ());
    TEST_EQUAL(0, observer.map_updated_calls(), ());
    pair<MwmSet::MwmLock, Index::UpdateStatus> const p = index.UpdateMap(testMapName);
    TEST(p.first.IsLocked(), ());
    TEST_EQUAL(Index::UPDATE_STATUS_OK, p.second, ());
    TEST_EQUAL(1, observer.map_update_is_ready_calls(), ());
    TEST_EQUAL(1, observer.map_updated_calls(), ());
  }

  // Tries to delete map in presence of active lock. Map should be
  // marked "to be removed" but can't be deleted.
  {
    MwmSet::MwmLock const lock = index.GetMwmLockByFileName(testMapName);
    TEST(lock.IsLocked(), ());

    TEST(!index.DeleteMap(testMapName), ());
    TEST_EQUAL(0, observer.map_deleted_calls(), ());
  }

  // Checks that observers are notified when all locks are destroyed.
  TEST_EQUAL(1, observer.map_deleted_calls(), ());

  index.RemoveObserver(observer);
}
