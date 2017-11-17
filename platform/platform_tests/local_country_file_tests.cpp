#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/set.hpp"

using namespace platform::tests_support;

namespace platform
{
namespace
{
template <typename T>
bool Contains(vector<T> const & v, T const & t)
{
  return find(v.begin(), v.end(), t) != v.end();
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

  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION, localFile.GetPath(MapOptions::Map), ());
  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION,
             localFile.GetPath(MapOptions::CarRouting), ());

  // Not synced with disk yet.
  TEST_EQUAL(MapOptions::Nothing, localFile.GetFiles(), ());

  // Any statement is true about elements of an empty set.
  TEST(localFile.OnDisk(MapOptions::Nothing), ());

  TEST(!localFile.OnDisk(MapOptions::Map), ());
  TEST(!localFile.OnDisk(MapOptions::CarRouting), ());
  TEST(!localFile.OnDisk(MapOptions::MapWithCarRouting), ());

  TEST_EQUAL("/test-dir", localFile.GetDirectory(), ());

  TEST_EQUAL(0, localFile.GetSize(MapOptions::Nothing), ());
  TEST_EQUAL(0, localFile.GetSize(MapOptions::Map), ());
  TEST_EQUAL(0, localFile.GetSize(MapOptions::CarRouting), ());
  TEST_EQUAL(0, localFile.GetSize(MapOptions::MapWithCarRouting), ());

  TEST_EQUAL(150309, localFile.GetVersion(), ());
}

// Creates test country map file and routing file and checks
// sync-with-disk functionality.
UNIT_TEST(LocalCountryFile_DiskFiles)
{
  Platform & platform = GetPlatform();

  CountryFile countryFile("TestCountry");
  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);

  for (int64_t version : {1, 150312})
  {
    LocalCountryFile localFile(platform.WritableDir(), countryFile, version);
    TEST(!localFile.OnDisk(MapOptions::Map), ());
    TEST(!localFile.OnDisk(MapOptions::CarRouting), ());
    TEST(!localFile.OnDisk(MapOptions::MapWithCarRouting), ());

    string const mapFileName = GetFileName(countryFile.GetName(), MapOptions::Map,
                                           version::FOR_TESTING_TWO_COMPONENT_MWM1);
    ScopedFile testMapFile(mapFileName, ScopedFile::Mode::Create);

    localFile.SyncWithDisk();
    TEST(localFile.OnDisk(MapOptions::Map), ());
    TEST(!localFile.OnDisk(MapOptions::CarRouting), ());
    TEST(!localFile.OnDisk(MapOptions::MapWithCarRouting), ());
    TEST_EQUAL(3, localFile.GetSize(MapOptions::Map), ());

    string const routingFileName = GetFileName(countryFile.GetName(), MapOptions::CarRouting,
                                               version::FOR_TESTING_TWO_COMPONENT_MWM1);
    ScopedFile testRoutingFile(routingFileName, ScopedFile::Mode::Create);

    localFile.SyncWithDisk();
    TEST(localFile.OnDisk(MapOptions::Map), ());
    TEST(localFile.OnDisk(MapOptions::CarRouting), ());
    TEST(localFile.OnDisk(MapOptions::MapWithCarRouting), ());
    TEST_EQUAL(3, localFile.GetSize(MapOptions::Map), ());
    TEST_EQUAL(7, localFile.GetSize(MapOptions::CarRouting), ());
    TEST_EQUAL(10, localFile.GetSize(MapOptions::MapWithCarRouting), ());

    localFile.DeleteFromDisk(MapOptions::MapWithCarRouting);
    TEST(!testMapFile.Exists(), (testMapFile, "wasn't deleted by LocalCountryFile."));
    testMapFile.Reset();

    TEST(!testRoutingFile.Exists(), (testRoutingFile, "wasn't deleted by LocalCountryFile."));
    testRoutingFile.Reset();
  }
}

UNIT_TEST(LocalCountryFile_CleanupMapFiles)
{
  Platform & platform = GetPlatform();
  string const mapsDir = platform.WritableDir();

  // Two fake directories for test country files and indexes.
  ScopedDir dir3("3");
  ScopedDir dir4("4");

  ScopedDir absentCountryIndexesDir(dir4, "Absent");
  ScopedDir irelandIndexesDir(dir4, "Ireland");

  CountryFile japanFile("Japan");
  CountryFile brazilFile("Brazil");
  CountryFile irelandFile("Ireland");

  LocalCountryFile japanLocalFile(mapsDir, japanFile, 0 /* version */);
  ScopedFile japanMapFile("Japan.mwm", ScopedFile::Mode::Create);

  LocalCountryFile brazilLocalFile(mapsDir, brazilFile, 0 /* version */);
  ScopedFile brazilMapFile("Brazil.mwm", ScopedFile::Mode::Create);

  LocalCountryFile irelandLocalFile(dir4.GetFullPath(), irelandFile, 4 /* version */);
  ScopedFile irelandMapFile(dir4, irelandFile, MapOptions::Map);

  // Check FindAllLocalMaps()
  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(4 /* latestVersion */, localFiles);
  TEST(!Contains(localFiles, japanLocalFile), (japanLocalFile, localFiles));
  TEST(!Contains(localFiles, brazilLocalFile), (brazilLocalFile, localFiles));
  TEST(Contains(localFiles, irelandLocalFile), (irelandLocalFile, localFiles));

  TEST(!japanMapFile.Exists(), (japanMapFile));
  japanMapFile.Reset();

  TEST(!brazilMapFile.Exists(), (brazilMapFile));
  brazilMapFile.Reset();

  irelandLocalFile.SyncWithDisk();
  TEST_EQUAL(MapOptions::Map, irelandLocalFile.GetFiles(), ());
  irelandLocalFile.DeleteFromDisk(MapOptions::Map);
  TEST(!irelandMapFile.Exists(), (irelandMapFile));
  irelandMapFile.Reset();

  TEST(!dir3.Exists(), ("Empty directory", dir3, "wasn't removed."));
  dir3.Reset();

  TEST(dir4.Exists(), ());

  TEST(!absentCountryIndexesDir.Exists(), ("Indexes for absent country weren't deleted."));
  absentCountryIndexesDir.Reset();

  TEST(irelandIndexesDir.Exists(), ());
}

UNIT_TEST(LocalCountryFile_CleanupPartiallyDownloadedFiles)
{
  ScopedDir oldDir("101009");
  ScopedDir latestDir("101010");

  ScopedFile toBeDeleted[] = {
      {"Ireland.mwm.ready", ScopedFile::Mode::Create},
      {"Netherlands.mwm.routing.downloading2", ScopedFile::Mode::Create},
      {"Germany.mwm.ready3", ScopedFile::Mode::Create},
      {"UK_England.mwm.resume4", ScopedFile::Mode::Create},
      {my::JoinFoldersToPath(oldDir.GetRelativePath(), "Russia_Central.mwm.downloading"),
       ScopedFile::Mode::Create}};
  ScopedFile toBeKept[] = {
      {"Italy.mwm", ScopedFile::Mode::Create},
      {"Spain.mwm", ScopedFile::Mode::Create},
      {"Spain.mwm.routing", ScopedFile::Mode::Create},
      {my::JoinFoldersToPath(latestDir.GetRelativePath(), "Russia_Southern.mwm.downloading"),
       ScopedFile::Mode::Create}};

  CleanupMapsDirectory(101010 /* latestVersion */);

  for (ScopedFile & file : toBeDeleted)
  {
    TEST(!file.Exists(), (file));
    file.Reset();
  }
  TEST(!oldDir.Exists(), (oldDir));
  oldDir.Reset();

  for (ScopedFile & file : toBeKept)
    TEST(file.Exists(), (file));
  TEST(latestDir.Exists(), (latestDir));
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

  ScopedDir testDir("test-dir");

  ScopedFile testIrelandMapFile(testDir, irelandFile, MapOptions::Map);
  ScopedFile testNetherlandsMapFile(testDir, netherlandsFile, MapOptions::Map);
  ScopedFile testNetherlandsRoutingFile(testDir, netherlandsFile, MapOptions::CarRouting);

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsInDirectoryAndCleanup(testDir.GetFullPath(), 150309 /* version */,
                                        -1 /* latestVersion */, localFiles);
  sort(localFiles.begin(), localFiles.end());
  for (LocalCountryFile & localFile : localFiles)
    localFile.SyncWithDisk();

  LocalCountryFile expectedIrelandFile(testDir.GetFullPath(), irelandFile, 150309);
  expectedIrelandFile.m_files = MapOptions::Map;

  LocalCountryFile expectedNetherlandsFile(testDir.GetFullPath(), netherlandsFile, 150309);
  expectedNetherlandsFile.m_files = MapOptions::MapWithCarRouting;

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

  ScopedDir testDir("10101");

  settings::Delete("LastMigration");

  ScopedFile testItalyMapFile(testDir, italyFile, MapOptions::Map);

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(10101 /* latestVersion */, localFiles);
  multiset<LocalCountryFile> localFilesSet(localFiles.begin(), localFiles.end());

  bool worldFound = false;
  bool worldCoastsFound = false;
  for (auto const & file : localFiles)
  {
    // With the new concepts, World mwm files have valid version.
    if (file.GetCountryName() == WORLD_FILE_NAME)
    {
      worldFound = true;
      TEST_NOT_EQUAL(0, file.GetVersion(), (file));
    }
    if (file.GetCountryName() == WORLD_COASTS_OBSOLETE_FILE_NAME)
    {
      worldCoastsFound = true;
      TEST_NOT_EQUAL(0, file.GetVersion(), (file));
    }
  }
  TEST(worldFound, ());
  TEST(worldCoastsFound, ());

  LocalCountryFile expectedItalyFile(testDir.GetFullPath(), italyFile, 10101);
  TEST_EQUAL(1, localFilesSet.count(expectedItalyFile), (localFiles));
}

UNIT_TEST(LocalCountryFile_PreparePlaceForCountryFiles)
{
  Platform & platform = GetPlatform();

  CountryFile italyFile("Italy");
  LocalCountryFile expectedItalyFile(platform.WritableDir(), italyFile, 0 /* version */);
  shared_ptr<LocalCountryFile> italyLocalFile =
      PreparePlaceForCountryFiles(0 /* version */, italyFile);
  TEST(italyLocalFile.get(), ());
  TEST_EQUAL(expectedItalyFile, *italyLocalFile, ());

  ScopedDir directoryForV1("1");

  CountryFile germanyFile("Germany");
  LocalCountryFile expectedGermanyFile(directoryForV1.GetFullPath(), germanyFile, 1 /* version */);
  shared_ptr<LocalCountryFile> germanyLocalFile =
      PreparePlaceForCountryFiles(1 /* version */, germanyFile);
  TEST(germanyLocalFile.get(), ());
  TEST_EQUAL(expectedGermanyFile, *germanyLocalFile, ());

  CountryFile franceFile("France");
  LocalCountryFile expectedFranceFile(directoryForV1.GetFullPath(), franceFile, 1 /* version */);
  shared_ptr<LocalCountryFile> franceLocalFile =
      PreparePlaceForCountryFiles(1 /* version */, franceFile);
  TEST(franceLocalFile.get(), ());
  TEST_EQUAL(expectedFranceFile, *franceLocalFile, ());
}

UNIT_TEST(LocalCountryFile_CountryIndexes)
{
  ScopedDir testDir("101010");

  CountryFile germanyFile("Germany");
  LocalCountryFile germanyLocalFile(testDir.GetFullPath(), germanyFile, 101010 /* version */);
  TEST_EQUAL(
      my::JoinFoldersToPath(germanyLocalFile.GetDirectory(), germanyFile.GetName()),
      CountryIndexes::IndexesDir(germanyLocalFile), ());
  CountryIndexes::PreparePlaceOnDisk(germanyLocalFile);

  string const bitsPath = CountryIndexes::GetPath(germanyLocalFile, CountryIndexes::Index::Bits);
  TEST(!Platform::IsFileExistsByFullPath(bitsPath), (bitsPath));
  {
    FileWriter writer(bitsPath);
    string const contents = "bits index";
    writer.Write(contents.data(), contents.size());
  }
  TEST(Platform::IsFileExistsByFullPath(bitsPath), (bitsPath));

  TEST(CountryIndexes::DeleteFromDisk(germanyLocalFile),
       ("Can't delete indexes for:", germanyLocalFile));

  TEST(!Platform::IsFileExistsByFullPath(bitsPath), (bitsPath));
}

UNIT_TEST(LocalCountryFile_DoNotDeleteUserFiles)
{
  my::ScopedLogLevelChanger const criticalLogLevel(LCRITICAL);

  ScopedDir testDir("101010");

  CountryFile germanyFile("Germany");
  LocalCountryFile germanyLocalFile(testDir.GetFullPath(), germanyFile, 101010 /* version */);
  CountryIndexes::PreparePlaceOnDisk(germanyLocalFile);

  string const userFilePath =
      my::JoinFoldersToPath(CountryIndexes::IndexesDir(germanyLocalFile), "user-data.txt");
  {
    FileWriter writer(userFilePath);
    string const data = "user data";
    writer.Write(data.data(), data.size());
  }
  TEST(!CountryIndexes::DeleteFromDisk(germanyLocalFile),
       ("Indexes dir should not be deleted for:", germanyLocalFile));

  TEST(my::DeleteFileX(userFilePath), ("Can't delete test file:", userFilePath));
  TEST(CountryIndexes::DeleteFromDisk(germanyLocalFile),
       ("Can't delete indexes for:", germanyLocalFile));
}

UNIT_TEST(LocalCountryFile_MakeTemporary)
{
  string const path = GetPlatform().WritablePathForFile("minsk-pass" DATA_FILE_EXTENSION);
  LocalCountryFile file = LocalCountryFile::MakeTemporary(path);
  TEST_EQUAL(file.GetPath(MapOptions::Map), path, ());
}

}  // namespace platform
