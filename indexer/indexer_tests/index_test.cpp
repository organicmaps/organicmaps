#include "../../testing/testing.hpp"
#include "../index.hpp"

#include "../../coding/file_name_utils.hpp"
#include "../../coding/internal/file_data.hpp"

#include "../../platform/platform.hpp"

#include "../../base/logging.hpp"
#include "../../base/macros.hpp"
#include "../../base/scope_guard.hpp"
#include "../../base/stl_add.hpp"

#include "../../std/bind.hpp"
#include "../../std/string.hpp"

void CheckedDeleteFile(string const & file)
{
  if (Platform::IsFileExistsByFullPath(file))
    CHECK(my::DeleteFileX(file), ("Can't remove file:", file));
}

class Observer : public Index::Observer
{
public:
  Observer(string const & file)
      : m_file(file), m_map_registered_calls(0), m_map_updated_calls(0), m_map_deleted_calls(0)
  {
  }

  // Index::Observer overrides:
  void OnMapRegistered(string const & file) override
  {
    CHECK_EQUAL(m_file, file, ());
    ++m_map_registered_calls;
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
  int map_updated_calls() const { return m_map_updated_calls; }
  int map_deleted_calls() const { return m_map_deleted_calls; }

private:
  string const m_file;
  int m_map_registered_calls;
  int m_map_updated_calls;
  int m_map_deleted_calls;
};

UNIT_TEST(Index_Parse)
{
  Index index;

  m2::RectD dummyRect;
  index.RegisterMap("minsk-pass" DATA_FILE_EXTENSION, dummyRect);

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
  m2::RectD dummyRect;

  // Check that observers are triggered after map registration.
  TEST_LESS_OR_EQUAL(0, index.RegisterMap(testMapName, dummyRect), ());
  TEST_EQUAL(1, observer.map_registered_calls(), ());

  // Check that map can't registered twice and observers aren't
  // triggered.
  TEST_EQUAL(-1, index.RegisterMap(testMapName, dummyRect), ());
  TEST_EQUAL(1, observer.map_registered_calls(), ());

  TEST(my::CopyFileX(testMapPath, testMapUpdatePath), ());
  MY_SCOPE_GUARD(testMapUpdateGuard, bind(&CheckedDeleteFile, testMapUpdatePath));

  // Check that observers are notified when map is deleted.
  TEST_EQUAL(0, observer.map_updated_calls(), ());
  TEST_LESS_OR_EQUAL(0, index.UpdateMap(testMapName, dummyRect), ());
  TEST_EQUAL(1, observer.map_updated_calls(), ());

  // Check that observers are notified when map is deleted.
  TEST_EQUAL(0, observer.map_deleted_calls(), ());
  TEST(index.DeleteMap(testMapName), ());
  TEST_EQUAL(1, observer.map_deleted_calls(), ());

  index.RemoveObserver(observer);
}
