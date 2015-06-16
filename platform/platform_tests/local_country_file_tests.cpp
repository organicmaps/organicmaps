#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/set.hpp"

namespace platform
{
namespace
{
template <typename T>
bool Contains(vector<T> const & v, T const & t)
{
  return find(v.begin(), v.end(), t) != v.end();
}

class ScopedTestDir
{
public:
  ScopedTestDir(string const & path) : m_path(path), m_reset(false)
  {
    Platform & platform = GetPlatform();
    Platform::EError ret = platform.MkDir(m_path);
    switch (ret)
    {
      case Platform::ERR_OK:
        break;
      case Platform::ERR_FILE_ALREADY_EXISTS:
        Platform::EFileType type;
        TEST_EQUAL(Platform::ERR_OK, Platform::GetFileType(m_path, type), ());
        TEST_EQUAL(Platform::FILE_TYPE_DIRECTORY, type, ());
        break;
      default:
        CHECK(false, ("Can't create directory:", m_path, "error:", ret));
        break;
    }
  }

  ~ScopedTestDir()
  {
    if (m_reset)
      return;

    Platform::EError ret = Platform::RmDir(m_path);
    switch (ret)
    {
      case Platform::ERR_OK:
        break;
      case Platform::ERR_FILE_DOES_NOT_EXIST:
        LOG(LWARNING, (m_path, "was deleted before destruction of ScopedTestDir."));
        break;
      case Platform::ERR_DIRECTORY_NOT_EMPTY:
        LOG(LWARNING, ("There are files in", m_path));
        break;
      default:
        LOG(LWARNING, ("Platform::RmDir() error:", ret));
        break;
    }
  }

  inline void Reset() { m_reset = true; }

  inline string const GetPath() const { return m_path; }

private:
  string const m_path;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedTestDir);
};

void CreateTestFile(string const & testFile, string const & contents)
{
  {
    FileWriter writer(testFile);
    writer.Write(contents.data(), contents.size());
  }
  TEST(Platform::IsFileExistsByFullPath(testFile), ("Can't create test file", testFile));
}

}  // namespace

// Checks that all unsigned numbers less than 10 ^ 18 can be parsed as
// a timestamp.
UNIT_TEST(LocalCountryFile_ParseVersion)
{
  int64_t version = 0;
  TEST(ParseVersion("1", version), ());
  TEST_EQUAL(version, 1, ());

  TEST(ParseVersion("141010", version), ());
  TEST_EQUAL(version, 141010, ());

  TEST(ParseVersion("150309", version), ());
  TEST_EQUAL(version, 150309, ());

  TEST(ParseVersion("999999999999999999", version), ());
  TEST_EQUAL(version, 999999999999999999, ());

  TEST(!ParseVersion("1000000000000000000", version), ());
  TEST(!ParseVersion("00000000000000000000000000000000123", version), ());

  TEST(!ParseVersion("", version), ());
  TEST(!ParseVersion("150309 ", version), ());
  TEST(!ParseVersion(" 150309", version), ());
  TEST(!ParseVersion("-150309", version), ());
  TEST(!ParseVersion("just string", version), ());
}

// Checks basic functionality of LocalCountryFile.
UNIT_TEST(LocalCountryFile_Smoke)
{
  CountryFile countryFile("TestCountry");
  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);

  LocalCountryFile localFile("/test-dir", countryFile, 150309);

  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION, localFile.GetPath(TMapOptions::EMap), ());
  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION,
             localFile.GetPath(TMapOptions::ECarRouting), ());

  // Not synced with disk yet.
  TEST_EQUAL(TMapOptions::ENothing, localFile.GetFiles(), ());

  // Any statement is true about elements of an empty set.
  TEST(localFile.OnDisk(TMapOptions::ENothing), ());

  TEST(!localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  TEST_EQUAL("/test-dir", localFile.GetDirectory(), ());

  TEST_EQUAL(0, localFile.GetSize(TMapOptions::ENothing), ());
  TEST_EQUAL(0, localFile.GetSize(TMapOptions::EMap), ());
  TEST_EQUAL(0, localFile.GetSize(TMapOptions::ECarRouting), ());
  TEST_EQUAL(0, localFile.GetSize(TMapOptions::EMapWithCarRouting), ());

  TEST_EQUAL(150309, localFile.GetVersion(), ());
}

// Creates test country map file and routing file and checks
// sync-with-disk functionality.
UNIT_TEST(LocalCountryFile_DiskFiles)
{
  Platform & platform = GetPlatform();

  CountryFile countryFile("TestCountry");
  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);

  string const testMapFile =
      my::JoinFoldersToPath(platform.WritableDir(), countryFile.GetNameWithExt(TMapOptions::EMap));
  string const testRoutingFile = my::JoinFoldersToPath(
      platform.WritableDir(), countryFile.GetNameWithExt(TMapOptions::ECarRouting));

  LocalCountryFile localFile(platform.WritableDir(), countryFile, 0 /* version */);
  TEST(!localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  CreateTestFile(testMapFile, "map");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMap), ());

  CreateTestFile(testRoutingFile, "routing");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMap), ());
  TEST(localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMap), ());
  TEST_EQUAL(7, localFile.GetSize(TMapOptions::ECarRouting), ());
  TEST_EQUAL(10, localFile.GetSize(TMapOptions::EMapWithCarRouting), ());

  localFile.DeleteFromDisk();
  TEST(!platform.IsFileExistsByFullPath(testMapFile),
       ("Map file", testMapFile, "wasn't deleted by LocalCountryFile."));
  TEST(!platform.IsFileExistsByFullPath(testRoutingFile),
       ("Routing file", testRoutingFile, "wasn't deleted by LocalCountryFile."));
}

UNIT_TEST(LocalCountryFile_DirectoryCleanup)
{
  Platform & platform = GetPlatform();
  string const mapsDir = platform.WritableDir();

  CountryFile japanFile("Japan");
  CountryFile brazilFile("Brazil");
  CountryFile irelandFile("Ireland");

  ScopedTestDir testDir1(my::JoinFoldersToPath(mapsDir, "1"));
  LocalCountryFile japanLocalFile(testDir1.GetPath(), japanFile, 1 /* version */);
  CreateTestFile(japanLocalFile.GetPath(TMapOptions::EMap), "Japan");

  ScopedTestDir testDir2(my::JoinFoldersToPath(mapsDir, "2"));
  LocalCountryFile brazilLocalFile(testDir2.GetPath(), brazilFile, 2 /* version */);
  CreateTestFile(brazilLocalFile.GetPath(TMapOptions::EMap), "Brazil");

  ScopedTestDir testDir3(my::JoinFoldersToPath(mapsDir, "3"));

  LocalCountryFile irelandLocalFile(testDir2.GetPath(), irelandFile, 2 /* version */);
  CreateTestFile(irelandLocalFile.GetPath(TMapOptions::EMap), "Ireland");

  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  TEST(Contains(localFiles, japanLocalFile), (japanLocalFile));
  TEST(Contains(localFiles, brazilLocalFile), (brazilLocalFile));
  TEST(Contains(localFiles, irelandLocalFile), (irelandLocalFile));

  CleanupMapsDirectory();

  japanLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, japanLocalFile.GetFiles(), ());
  TEST(!Platform::IsFileExistsByFullPath(testDir1.GetPath()), ("Empty directory wasn't removed."));
  testDir1.Reset();

  brazilLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, brazilLocalFile.GetFiles(), ());

  irelandLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::EMap, irelandLocalFile.GetFiles(), ());
  irelandLocalFile.DeleteFromDisk();

  TEST(!Platform::IsFileExistsByFullPath(testDir3.GetPath()), ("Empty directory wasn't removed."));
  testDir3.Reset();
}

// Creates test-dir and following files:
// * test-dir/Ireland.mwm
// * test-dir/Netherlands.mwm
// * test-dir/Netherlands.mwm.routing
// After that, checks that FindAllLocalMapsInDirectory() correctly finds all created files.
UNIT_TEST(LocalCountryFile_DirectoryLookup)
{
  // This tests creates a map file for Ireland and map + routing files
  // for Netherlands in a test directory.
  CountryFile const irelandFile("Ireland");
  CountryFile const netherlandsFile("Netherlands");

  Platform & platform = GetPlatform();

  ScopedTestDir testDir(my::JoinFoldersToPath(platform.WritableDir(), "test-dir"));

  string const testIrelandMapFile =
      my::JoinFoldersToPath(testDir.GetPath(), irelandFile.GetNameWithExt(TMapOptions::EMap));
  CreateTestFile(testIrelandMapFile, "Ireland-map");
  MY_SCOPE_GUARD(removeTestIrelandMapFile, bind(&FileWriter::DeleteFileX, testIrelandMapFile));

  string const testNetherlandsMapFile =
      my::JoinFoldersToPath(testDir.GetPath(), netherlandsFile.GetNameWithExt(TMapOptions::EMap));
  CreateTestFile(testNetherlandsMapFile, "Netherlands-map");
  MY_SCOPE_GUARD(removeTestNetherlandsMapFile,
                 bind(&FileWriter::DeleteFileX, testNetherlandsMapFile));

  string const testNetherlandsRoutingFile = my::JoinFoldersToPath(
      testDir.GetPath(), netherlandsFile.GetNameWithExt(TMapOptions::ECarRouting));
  CreateTestFile(testNetherlandsRoutingFile, "Netherlands-routing");
  MY_SCOPE_GUARD(removeTestNetherlandsRoutingFile,
                 bind(&FileWriter::DeleteFileX, testNetherlandsRoutingFile));

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsInDirectory(testDir.GetPath(), 150309, localFiles);
  sort(localFiles.begin(), localFiles.end());
  for (LocalCountryFile & localFile : localFiles)
    localFile.SyncWithDisk();

  LocalCountryFile expectedIrelandFile(testDir.GetPath(), irelandFile, 150309);
  expectedIrelandFile.m_files = TMapOptions::EMap;

  LocalCountryFile expectedNetherlandsFile(testDir.GetPath(), netherlandsFile, 150309);
  expectedNetherlandsFile.m_files = TMapOptions::EMapWithCarRouting;

  vector<LocalCountryFile> expectedLocalFiles = {expectedIrelandFile, expectedNetherlandsFile};
  sort(expectedLocalFiles.begin(), expectedLocalFiles.end());
  TEST_EQUAL(expectedLocalFiles, localFiles, ());
}

// Creates directory 010101 and 010101/Italy.mwm file.  After that,
// checks that this file will be recognized as a map file for Italy
// with version 010101. Also, checks that World.mwm and
// WorldCoasts.mwm exist in writable dir.
UNIT_TEST(LocalCountryFile_AllLocalFilesLookup)
{
  CountryFile const italyFile("Italy");

  Platform & platform = GetPlatform();

  ScopedTestDir testDir(my::JoinFoldersToPath(platform.WritableDir(), "010101"));

  string const testItalyMapFile =
      my::JoinFoldersToPath(testDir.GetPath(), italyFile.GetNameWithExt(TMapOptions::EMap));
  CreateTestFile(testItalyMapFile, "Italy-map");
  MY_SCOPE_GUARD(remoteTestItalyMapFile, bind(&FileWriter::DeleteFileX, testItalyMapFile));

  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  multiset<LocalCountryFile> localFilesSet(localFiles.begin(), localFiles.end());

  LocalCountryFile expectedWorldFile(platform.WritableDir(), CountryFile(WORLD_FILE_NAME),
                                     0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldFile), ());

  LocalCountryFile expectedWorldCoastsFile(platform.WritableDir(),
                                           CountryFile(WORLD_COASTS_FILE_NAME), 0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldCoastsFile), ());

  LocalCountryFile expectedItalyFile(testDir.GetPath(), italyFile, 10101);
  TEST_EQUAL(1, localFilesSet.count(expectedItalyFile), ());
}

UNIT_TEST(LocalCountryFile_PreparePlaceForCountryFiles)
{
  Platform & platform = GetPlatform();

  CountryFile italyFile("Italy");
  LocalCountryFile expectedItalyFile(platform.WritableDir(), italyFile, 0 /* version */);
  shared_ptr<LocalCountryFile> italyLocalFile =
      PreparePlaceForCountryFiles(italyFile, 0 /* version */);
  TEST(italyLocalFile.get(), ());
  TEST_EQUAL(expectedItalyFile, *italyLocalFile, ());

  ScopedTestDir directoryForV1(my::JoinFoldersToPath(platform.WritableDir(), "1"));

  CountryFile germanyFile("Germany");
  LocalCountryFile expectedGermanyFile(directoryForV1.GetPath(), germanyFile, 1 /* version */);
  shared_ptr<LocalCountryFile> germanyLocalFile =
      PreparePlaceForCountryFiles(germanyFile, 1 /* version */);
  TEST(germanyLocalFile.get(), ());
  TEST_EQUAL(expectedGermanyFile, *germanyLocalFile, ());

  CountryFile franceFile("France");
  LocalCountryFile expectedFranceFile(directoryForV1.GetPath(), franceFile, 1 /* version */);
  shared_ptr<LocalCountryFile> franceLocalFile =
      PreparePlaceForCountryFiles(franceFile, 1 /* version */);
  TEST(franceLocalFile.get(), ());
  TEST_EQUAL(expectedFranceFile, *franceLocalFile, ());
}
}  // namespace platform
