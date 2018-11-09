#include "testing/testing.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "defines.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/bind.hpp"
#include "std/initializer_list.hpp"
#include "std/set.hpp"

namespace
{
char const * TEST_FILE_NAME = "some_temporary_unit_test_file.tmp";

void CheckFilesPresence(string const & baseDir, unsigned typeMask,
                        initializer_list<pair<string, size_t>> const & files)
{
  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(baseDir, typeMask, fwts);

  multiset<string> filesSet;
  for (auto const & fwt : fwts)
    filesSet.insert(fwt.first);

  for (auto const & file : files)
    TEST_EQUAL(filesSet.count(file.first), file.second, (file.first, file.second));
}
}  // namespace

UNIT_TEST(WritableDir)
{
  string const path = GetPlatform().WritableDir() + TEST_FILE_NAME;

  try
  {
    base::FileData f(path, base::FileData::OP_WRITE_TRUNCATE);
  }
  catch (Writer::OpenException const &)
  {
    LOG(LCRITICAL, ("Can't create file", path));
    return;
  }

  base::DeleteFileX(path);
}

UNIT_TEST(WritablePathForFile)
{
  Platform & pl = GetPlatform();
  string const p1 = pl.WritableDir() + TEST_FILE_NAME;
  string const p2 = pl.WritablePathForFile(TEST_FILE_NAME);
  TEST_EQUAL(p1, p2, ());
}

UNIT_TEST(GetReader)
{
  char const * NON_EXISTING_FILE = "mgbwuerhsnmbui45efhdbn34.tmp";
  char const * arr[] = {
    "resources-mdpi_clear/symbols.sdf",
    "classificator.txt",
    "minsk-pass.mwm"
  };

  Platform & p = GetPlatform();
  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    ReaderPtr<Reader> r = p.GetReader(arr[i]);
    TEST_GREATER(r.Size(), 0, ("File should exist!"));
  }

  bool wasException = false;
  try
  {
    ReaderPtr<Reader> r = p.GetReader(NON_EXISTING_FILE);
  }
  catch (FileAbsentException const &)
  {
    wasException = true;
  }
  TEST_EQUAL(wasException, true, ());
}

UNIT_TEST(GetFilesInDir_Smoke)
{
  Platform & pl = GetPlatform();
  Platform::FilesList files1, files2;

  string const dir = pl.ResourcesDir();

  pl.GetFilesByExt(dir, DATA_FILE_EXTENSION, files1);
  TEST_GREATER(files1.size(), 0, (dir, "folder should contain some data files"));

  pl.GetFilesByRegExp(dir, ".*\\" DATA_FILE_EXTENSION "$", files2);
  TEST_EQUAL(files1, files2, ());

  files1.clear();
  pl.GetFilesByExt(dir, ".dsa", files1);
  TEST_EQUAL(files1.size(), 0, ());
}

UNIT_TEST(DirsRoutines)
{
  string const baseDir = GetPlatform().WritableDir();
  string const testDir = base::JoinFoldersToPath(baseDir, "test-dir");
  string const testFile = base::JoinFoldersToPath(testDir, "test-file");

  TEST(!Platform::IsFileExistsByFullPath(testDir), ());
  TEST_EQUAL(Platform::MkDir(testDir), Platform::ERR_OK, ());

  TEST(Platform::IsFileExistsByFullPath(testDir), ());
  TEST(Platform::IsDirectoryEmpty(testDir), ());

  {
    FileWriter writer(testFile);
  }
  TEST(!Platform::IsDirectoryEmpty(testDir), ());
  FileWriter::DeleteFileX(testFile);

  TEST_EQUAL(Platform::RmDir(testDir), Platform::ERR_OK, ());

  TEST(!Platform::IsFileExistsByFullPath(testDir), ());
}

UNIT_TEST(GetFilesByType)
{
  string const kTestDirBaseName = "test-dir";
  string const kTestFileBaseName = "test-file";

  string const baseDir = GetPlatform().WritableDir();

  string const testDir = base::JoinFoldersToPath(baseDir, kTestDirBaseName);
  TEST_EQUAL(Platform::MkDir(testDir), Platform::ERR_OK, ());
  SCOPE_GUARD(removeTestDir, bind(&Platform::RmDir, testDir));

  string const testFile = base::JoinFoldersToPath(baseDir, kTestFileBaseName);
  TEST(!Platform::IsFileExistsByFullPath(testFile), ());
  {
    FileWriter writer(testFile);
  }
  TEST(Platform::IsFileExistsByFullPath(testFile), ());
  SCOPE_GUARD(removeTestFile, bind(FileWriter::DeleteFileX, testFile));

  CheckFilesPresence(baseDir, Platform::FILE_TYPE_DIRECTORY,
  {{
     kTestDirBaseName, 1 /* present */
   },
   {
     kTestFileBaseName, 0 /* not present */
   }});
  CheckFilesPresence(baseDir, Platform::FILE_TYPE_REGULAR,
  {{
     kTestDirBaseName, 0 /* not present */
   },
   {
     kTestFileBaseName, 1 /* present */
   }});
  CheckFilesPresence(baseDir, Platform::FILE_TYPE_DIRECTORY | Platform::FILE_TYPE_REGULAR,
  {{
     kTestDirBaseName, 1 /* present */
   },
   {
     kTestFileBaseName, 1 /* present */
   }});
}

UNIT_TEST(GetFileSize)
{
  Platform & pl = GetPlatform();
  uint64_t size = 123141;
  TEST(!pl.GetFileSizeByName("adsmngfuwrbfyfwe", size), ());
  TEST(!pl.IsFileExistsByFullPath("adsmngfuwrbfyfwe"), ());

  string const fileName = pl.WritablePathForFile(TEST_FILE_NAME);
  {
    FileWriter testFile(fileName);
    testFile.Write("HOHOHO", 6);
  }
  size = 0;
  TEST(Platform::GetFileSizeByFullPath(fileName, size), ());
  TEST_EQUAL(size, 6, ());

  FileWriter::DeleteFileX(fileName);

  {
    FileWriter testFile(pl.WritablePathForFile(TEST_FILE_NAME));
    testFile.Write("HOHOHO", 6);
  }
  size = 0;
  TEST(pl.GetFileSizeByName(TEST_FILE_NAME, size), ());
  TEST_EQUAL(size, 6, ());

  FileWriter::DeleteFileX(pl.WritablePathForFile(TEST_FILE_NAME));
}

UNIT_TEST(CpuCores)
{
  int const coresNum = GetPlatform().CpuCores();
  TEST_GREATER(coresNum, 0, ());
  TEST_LESS_OR_EQUAL(coresNum, 128, ());
}

UNIT_TEST(GetWritableStorageStatus)
{
  TEST_EQUAL(Platform::STORAGE_OK, GetPlatform().GetWritableStorageStatus(100000), ());
  TEST_EQUAL(Platform::NOT_ENOUGH_SPACE, GetPlatform().GetWritableStorageStatus(uint64_t(-1)), ());
}

UNIT_TEST(RmDirRecursively)
{
  string const testDir1 = base::JoinFoldersToPath(GetPlatform().WritableDir(), "test_dir1");
  TEST_EQUAL(Platform::MkDir(testDir1), Platform::ERR_OK, ());
  SCOPE_GUARD(removeTestDir1, bind(&Platform::RmDir, testDir1));

  string const testDir2 = base::JoinFoldersToPath(testDir1, "test_dir2");
  TEST_EQUAL(Platform::MkDir(testDir2), Platform::ERR_OK, ());
  SCOPE_GUARD(removeTestDir2, bind(&Platform::RmDir, testDir2));

  string const filePath = base::JoinFoldersToPath(testDir2, "test_file");
  {
    FileWriter testFile(filePath);
    testFile.Write("HOHOHO", 6);
  }
  SCOPE_GUARD(removeTestFile, bind(&base::DeleteFileX, filePath));

  TEST(Platform::IsFileExistsByFullPath(filePath), ());
  TEST(Platform::IsFileExistsByFullPath(testDir1), ());
  TEST(Platform::IsFileExistsByFullPath(testDir2), ());

  TEST_EQUAL(Platform::ERR_DIRECTORY_NOT_EMPTY, Platform::RmDir(testDir1), ());

  TEST(Platform::IsFileExistsByFullPath(filePath), ());
  TEST(Platform::IsFileExistsByFullPath(testDir1), ());
  TEST(Platform::IsFileExistsByFullPath(testDir2), ());

  TEST(Platform::RmDirRecursively(testDir1), ());

  TEST(!Platform::IsFileExistsByFullPath(filePath), ());
  TEST(!Platform::IsFileExistsByFullPath(testDir1), ());
  TEST(!Platform::IsFileExistsByFullPath(testDir2), ());
}

UNIT_TEST(IsSingleMwm)
{
  TEST(version::IsSingleMwm(version::FOR_TESTING_SINGLE_MWM1), ());
  TEST(version::IsSingleMwm(version::FOR_TESTING_SINGLE_MWM_LATEST), ());
  TEST(!version::IsSingleMwm(version::FOR_TESTING_TWO_COMPONENT_MWM1), ());
  TEST(!version::IsSingleMwm(version::FOR_TESTING_TWO_COMPONENT_MWM2), ());
}

UNIT_TEST(MkDirRecursively)
{
  using namespace platform::tests_support;
  auto const writablePath = GetPlatform().WritableDir();
  auto const workPath = base::JoinPath(writablePath, "MkDirRecursively");
  auto const resetDir =  [](std::string const & path) {
    if (Platform::IsFileExistsByFullPath(path) && !Platform::RmDirRecursively(path))
       return false;

    return Platform::MkDirChecked(path);
  };

  CHECK(resetDir(workPath), ());
  auto const path = base::JoinPath(workPath, "test1", "test2", "test3");
  TEST(Platform::MkDirRecursively(path), ());
  TEST(Platform::IsFileExistsByFullPath(path), ());
  TEST(Platform::IsDirectory(path), ());

  CHECK(resetDir(workPath), ());
  auto const filePath = base::JoinPath(workPath, "test1");
  FileWriter testFile(filePath);
  SCOPE_GUARD(removeTestFile, bind(&base::DeleteFileX, filePath));

  TEST(!Platform::MkDirRecursively(path), ());
  TEST(!Platform::IsFileExistsByFullPath(path), ());

  CHECK(Platform::RmDirRecursively(workPath), ());
}
