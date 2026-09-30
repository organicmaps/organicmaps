#include "testing/testing.hpp"

#include "indexer/interval_index.hpp"
#include "indexer/terrain/terrain_serdes.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <string>
#include <vector>

namespace twm_set_tests
{
namespace
{
terrain::TwmFile WriteTerrainFile(std::string const & dir, int64_t version, std::string const & name = "N00E000")
{
  std::string const versionDir = base::JoinPath(dir, std::to_string(version));
  TEST(Platform::MkDirRecursively(versionDir), (versionDir));
  std::string const path = base::JoinPath(versionDir, name + ".twm");
  terrain::TwmHeader header;
  header.m_geometries = {{17, 0}};
  header.m_limitLB = PointDToPointU({0, 0}, header.m_coordBits);
  header.m_limitRT = PointDToPointU({1, 1}, header.m_coordBits);
  {
    FilesContainerW container(path);
    {
      auto writer = container.GetWriter(terrain::kHeaderTag);
      header.Serialize(*writer);
    }
    {
      terrain::MeshGrid grid;
      grid.m_samplesPerDegree = 1;
      grid.m_colX = {header.m_limitLB.x, header.m_limitRT.x};
      grid.m_rowY = {header.m_limitLB.y, header.m_limitRT.y};
      auto writer = container.GetWriter(terrain::kGridTag);
      terrain::SerializeGrid(*writer, {grid}, header.m_coordBits);
    }
    {
      auto writer = container.GetWriter(terrain::kFreqTag);
      for (size_t i = 0; i < terrain::impl::kContextCount; ++i)
        WriteVarUint(*writer, 0U);
    }
    {
      // Empty meshes need no feature records or geometry sections.
      IntervalIndexBase::Header const indexHeader{IntervalIndexBase::kVersion, 0, 0, 0};
      auto writer = container.GetWriter(terrain::kIndexTag);
      writer->Write(&indexHeader, sizeof(indexHeader));
    }
  }
  terrain::TwmFile file;
  TEST_EQUAL(terrain::TwmSet::ReadFile(path, version, file), terrain::TwmSet::RegResult::Success, (path));
  return file;
}

void CheckFile(terrain::TwmFile const & actual, terrain::TwmFile const & expected)
{
  TEST_EQUAL(actual.m_id, expected.m_id, ());
  TEST_EQUAL(actual.m_version, expected.m_version, ());
  TEST_EQUAL(actual.m_path, expected.m_path, ());
  TEST_EQUAL(actual.m_rect, expected.m_rect, ());
}

struct Observer : terrain::TwmSet::Observer
{
  void OnTerrainDeregistered(terrain::TwmFile const & file) override { m_deregistered.push_back(file); }
  std::vector<terrain::TwmFile> m_deregistered;
};
}  // namespace

UNIT_TEST(TwmSet_IdLookupAndDeregister)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "twm_set_id_test");
  platform::tests_support::ScopedDirCleanup const cleanup(dir);
  auto const file = WriteTerrainFile(dir, 1);
  Observer observer;
  terrain::TwmSet set;
  set.AddObserver(observer);

  auto const [id, result] = set.Register(file);
  TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(set.GetId(file.m_id), id, ());
  TEST(set.GetId(file.m_path).IsNull(), ());
  CheckFile(id.GetInfo()->GetFile(), file);
  TEST(set.IsFileAlive(file), ());
  TEST_EQUAL(set.Register(file).second, terrain::TwmSet::RegResult::AlreadyRegistered, ());

  TEST(!set.Deregister("missing"), ());
  TEST(set.Deregister(file.m_id), ());
  TEST(!id.IsAlive(), ());
  TEST(!set.IsFileAlive(file), ());
  TEST(set.GetId(file.m_id).IsNull(), ());
  TEST(!set.Deregister(file.m_id), ());
  TEST_EQUAL(observer.m_deregistered.size(), 1, ());
  CheckFile(observer.m_deregistered.front(), file);
  TEST(Platform::IsFileExistsByFullPath(file.m_path), ());
}

UNIT_TEST(TwmSet_ReplaceWhileOldHandleIsHeld)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "twm_set_replace_test");
  platform::tests_support::ScopedDirCleanup const cleanup(dir);
  auto const oldFile = WriteTerrainFile(dir, 1);
  // Both version changes and grid changes can replace a still-open old block.
  for (bool const sameId : {true, false})
  {
    auto const newFile = WriteTerrainFile(dir, 2, sameId ? oldFile.m_id : "N00E001");
    Observer observer;
    terrain::TwmSet set;
    set.AddObserver(observer);
    auto const oldId = set.Register(oldFile).first;
    TEST_EQUAL(set.Register(newFile).second, terrain::TwmSet::RegResult::Overlapping, (sameId));

    auto oldHandle = set.GetHandleById(oldId);
    TEST(oldHandle.IsAlive(), (sameId));
    TEST(!set.Deregister(oldFile.m_id), (sameId));
    TEST(observer.m_deregistered.empty(), (sameId));
    TEST(set.IsFileAlive(oldFile), (sameId));
    TEST(!set.IsFileAlive(newFile), (sameId));
    auto const [newId, result] = set.Register(newFile);
    TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, (sameId));
    TEST_EQUAL(set.GetId(newFile.m_id), newId, (sameId));
    std::vector<terrain::TwmId> visible;
    set.GetBlocksByRect(newFile.m_rect, visible);
    TEST_EQUAL(visible, (std::vector<terrain::TwmId>{newId}), (sameId));
    TEST(oldHandle.IsAlive(), (sameId));
    TEST(set.IsFileAlive(oldFile), (sameId));
    TEST(set.IsFileAlive(newFile), (sameId));

    oldHandle = {};
    TEST(!oldId.IsAlive(), (sameId));
    TEST(!set.IsFileAlive(oldFile), (sameId));
    TEST(set.IsFileAlive(newFile), (sameId));
    TEST_EQUAL(observer.m_deregistered.size(), 1, (sameId));
    CheckFile(observer.m_deregistered.front(), oldFile);
    TEST_EQUAL(set.GetId(newFile.m_id), newId, (sameId));
    TEST(newId.GetInfo()->IsRegistered(), (sameId));
    TEST(set.GetHandleById(newId).IsAlive(), (sameId));

    TEST(set.Deregister(newFile.m_id), (sameId));
    TEST_EQUAL(observer.m_deregistered.size(), 2, (sameId));
    CheckFile(observer.m_deregistered.back(), newFile);
  }
}

UNIT_TEST(TwmSet_ResurrectMarkedFile)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "twm_set_resurrect_test");
  platform::tests_support::ScopedDirCleanup const cleanup(dir);
  auto const file = WriteTerrainFile(dir, 1);
  Observer observer;
  terrain::TwmSet set;
  set.AddObserver(observer);
  auto const id = set.Register(file).first;
  auto handle = set.GetHandleById(id);
  TEST(handle.IsAlive(), ());
  TEST(!set.Deregister(file.m_id), ());
  TEST(!id.GetInfo()->IsRegistered(), ());
  TEST(set.IsFileAlive(file), ());
  TEST_EQUAL(set.Register(file).second, terrain::TwmSet::RegResult::AlreadyRegistered, ());
  handle = {};
  TEST(observer.m_deregistered.empty(), ());
  TEST_EQUAL(set.GetId(file.m_id), id, ());
  TEST(id.GetInfo()->IsRegistered(), ());
  TEST(set.Deregister(file.m_id), ());
  TEST_EQUAL(observer.m_deregistered.size(), 1, ());
  CheckFile(observer.m_deregistered.front(), file);
}

UNIT_TEST(TwmSet_ReregisterCondemnedFile)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "twm_set_condemn_test");
  platform::tests_support::ScopedDirCleanup const cleanup(dir);
  auto const file = WriteTerrainFile(dir, 1);
  Observer observer;
  terrain::TwmSet set;
  set.AddObserver(observer);
  auto const id = set.Register(file).first;
  auto handle = set.GetHandleById(id);
  TEST(handle.IsAlive(), ());

  // Late corruption detection must preserve readers until their last unlock.
  set.Condemn({id});
  TEST(!id.GetInfo()->IsRegistered(), ());
  TEST(set.IsFileAlive(file), ());
  TEST(observer.m_deregistered.empty(), ());
  handle = {};
  TEST(!id.IsAlive(), ());
  TEST(!set.IsFileAlive(file), ());
  TEST_EQUAL(observer.m_deregistered.size(), 1, ());
  CheckFile(observer.m_deregistered.front(), file);

  // Storage removes the deregistered file and publishes a download at the same path.
  TEST(Platform::RemoveFileIfExists(file.m_path), ());
  auto const downloaded = WriteTerrainFile(dir, 1);
  CheckFile(downloaded, file);
  auto const [newId, result] = set.Register(downloaded);
  TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, ());
  TEST_NOT_EQUAL(newId, id, ());
  auto newHandle = set.GetHandleById(newId);
  TEST(newHandle.IsAlive(), ());
  TEST_EQUAL(newHandle.GetValue()->GetReader().GetHeader().GetLimitRect(), downloaded.m_rect, ());
}

UNIT_TEST(TwmSet_ReregisterAfterReaderFailure)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "twm_set_reader_failure_test");
  platform::tests_support::ScopedDirCleanup const cleanup(dir);
  auto const file = WriteTerrainFile(dir, 1);
  {
    // The header remains readable, but constructing the full reader must fail.
    auto const header = terrain::Reader(FilesContainerR(file.m_path)).GetHeader();
    FilesContainerW container(file.m_path);
    auto writer = container.GetWriter(terrain::kHeaderTag);
    header.Serialize(*writer);
  }
  Observer observer;
  terrain::TwmSet set;
  set.AddObserver(observer);
  auto const id = set.Register(file).first;
  {
    base::ScopedLogAbortLevelChanger const expectedReaderFailure(LCRITICAL);
    TEST(!set.GetHandleById(id).IsAlive(), ());
  }
  TEST(!id.IsAlive(), ());
  TEST_EQUAL(observer.m_deregistered.size(), 1, ());

  TEST(Platform::RemoveFileIfExists(file.m_path), ());
  auto const downloaded = WriteTerrainFile(dir, 1);
  auto const [newId, result] = set.Register(downloaded);
  TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, ());
  TEST(set.GetHandleById(newId).IsAlive(), ());
}
}  // namespace twm_set_tests
