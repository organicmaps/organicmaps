#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

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

  LocalCountryFile localFile(platform.WritableDir(), countryFile, 0 /* version */);
  TEST(!localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  ScopedFile testMapFile(countryFile.GetNameWithExt(TMapOptions::EMap), "map");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMap), ());

  ScopedFile testRoutingFile(countryFile.GetNameWithExt(TMapOptions::ECarRouting), "routing");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMap), ());
  TEST(localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMap), ());
  TEST_EQUAL(7, localFile.GetSize(TMapOptions::ECarRouting), ());
  TEST_EQUAL(10, localFile.GetSize(TMapOptions::EMapWithCarRouting), ());

  localFile.DeleteFromDisk(TMapOptions::EMapWithCarRouting);
  TEST(!testMapFile.Exists(), (testMapFile, "wasn't deleted by LocalCountryFile."));
  testMapFile.Reset();

  TEST(!testRoutingFile.Exists(), (testRoutingFile, "wasn't deleted by LocalCountryFile."));
  testRoutingFile.Reset();
}

UNIT_TEST(LocalCountryFile_CleanupMapFiles)
{
  Platform & platform = GetPlatform();
  string const mapsDir = platform.WritableDir();

  CountryFile japanFile("Japan");
  CountryFile brazilFile("Brazil");
  CountryFile irelandFile("Ireland");

  ScopedDir testDir1("1");
  LocalCountryFile japanLocalFile(testDir1.GetFullPath(), japanFile, 1 /* version */);
  ScopedFile japanMapFile(testDir1, japanFile, TMapOptions::EMap, "Japan");

  ScopedDir testDir2("2");
  LocalCountryFile brazilLocalFile(testDir2.GetFullPath(), brazilFile, 2 /* version */);
  ScopedFile brazilMapFile(testDir2, brazilFile, TMapOptions::EMap, "Brazil");
  LocalCountryFile irelandLocalFile(testDir2.GetFullPath(), irelandFile, 2 /* version */);
  ScopedFile irelandMapFile(testDir2, irelandFile, TMapOptions::EMap, "Ireland");

  ScopedDir testDir3("3");

  // Check that FindAllLocalMaps()
  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  TEST(Contains(localFiles, japanLocalFile), (japanLocalFile));
  TEST(Contains(localFiles, brazilLocalFile), (brazilLocalFile));
  TEST(Contains(localFiles, irelandLocalFile), (irelandLocalFile));

  CleanupMapsDirectory();

  japanLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, japanLocalFile.GetFiles(), ());
  TEST(!testDir1.Exists(), ("Empty directory", testDir1, "wasn't removed."));
  testDir1.Reset();
  TEST(!japanMapFile.Exists(), (japanMapFile));
  japanMapFile.Reset();

  brazilLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, brazilLocalFile.GetFiles(), ());
  TEST(!brazilMapFile.Exists(), (brazilMapFile));
  brazilMapFile.Reset();

  irelandLocalFile.SyncWithDisk();
  TEST_EQUAL(TMapOptions::EMap, irelandLocalFile.GetFiles(), ());
  irelandLocalFile.DeleteFromDisk(TMapOptions::EMap);
  TEST(!irelandMapFile.Exists(), (irelandMapFile));
  irelandMapFile.Reset();

  TEST(!testDir3.Exists(), ("Empty directory", testDir3, "wasn't removed."));
  testDir3.Reset();
}

UNIT_TEST(LocalCountryFile_CleanupPartiallyDownloadedFiles)
{
  ScopedFile toBeDeleted[] = {{"Ireland.mwm.ready", "Ireland"},
                              {"Netherlands.mwm.routing.downloading2", "Netherlands"},
                              {"Germany.mwm.ready3", "Germany"},
                              {"UK_England.mwm.resume4", "UK"}};
  ScopedFile toBeKept[] = {
      {"Italy.mwm", "Italy"}, {"Spain.mwm", "Spain map"}, {"Spain.mwm.routing", "Spain routing"}};

  CleanupMapsDirectory();

  for (ScopedFile & file : toBeDeleted)
  {
    TEST(!file.Exists(), (file));
    file.Reset();
  }

  for (ScopedFile & file : toBeKept)
    TEST(file.Exists(), (file));
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

  ScopedFile testIrelandMapFile(testDir, irelandFile, TMapOptions::EMap, "Ireland-map");
  ScopedFile testNetherlandsMapFile(testDir, netherlandsFile, TMapOptions::EMap, "Netherlands-map");
  ScopedFile testNetherlandsRoutingFile(testDir, netherlandsFile, TMapOptions::ECarRouting,
                                        "Netherlands-routing");

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsInDirectory(testDir.GetFullPath(), 150309, localFiles);
  sort(localFiles.begin(), localFiles.end());
  for (LocalCountryFile & localFile : localFiles)
    localFile.SyncWithDisk();

  LocalCountryFile expectedIrelandFile(testDir.GetFullPath(), irelandFile, 150309);
  expectedIrelandFile.m_files = TMapOptions::EMap;

  LocalCountryFile expectedNetherlandsFile(testDir.GetFullPath(), netherlandsFile, 150309);
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

  ScopedDir testDir("010101");
  ScopedFile testItalyMapFile(testDir, italyFile, TMapOptions::EMap, "Italy-map");

  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  multiset<LocalCountryFile> localFilesSet(localFiles.begin(), localFiles.end());

  LocalCountryFile expectedWorldFile(platform.WritableDir(), CountryFile(WORLD_FILE_NAME),
                                     0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldFile), ());

  LocalCountryFile expectedWorldCoastsFile(platform.WritableDir(),
                                           CountryFile(WORLD_COASTS_FILE_NAME), 0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldCoastsFile), ());

  LocalCountryFile expectedItalyFile(testDir.GetFullPath(), italyFile, 10101);
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

  ScopedDir directoryForV1("1");

  CountryFile germanyFile("Germany");
  LocalCountryFile expectedGermanyFile(directoryForV1.GetFullPath(), germanyFile, 1 /* version */);
  shared_ptr<LocalCountryFile> germanyLocalFile =
      PreparePlaceForCountryFiles(germanyFile, 1 /* version */);
  TEST(germanyLocalFile.get(), ());
  TEST_EQUAL(expectedGermanyFile, *germanyLocalFile, ());

  CountryFile franceFile("France");
  LocalCountryFile expectedFranceFile(directoryForV1.GetFullPath(), franceFile, 1 /* version */);
  shared_ptr<LocalCountryFile> franceLocalFile =
      PreparePlaceForCountryFiles(franceFile, 1 /* version */);
  TEST(franceLocalFile.get(), ());
  TEST_EQUAL(expectedFranceFile, *franceLocalFile, ());
}

UNIT_TEST(LocalCountryFile_CountryIndexes)
{
  ScopedDir testDir("101010");

  CountryFile germanyFile("Germany");
  LocalCountryFile germanyLocalFile(testDir.GetFullPath(), germanyFile, 101010 /* version */);
  TEST_EQUAL(
      my::JoinFoldersToPath(germanyLocalFile.GetDirectory(), germanyFile.GetNameWithoutExt()),
      CountryIndexes::IndexesDir(germanyLocalFile), ());
  TEST(CountryIndexes::PreparePlaceOnDisk(germanyLocalFile),
       ("Can't prepare place for:", germanyLocalFile));

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
  my::LogLevel oldLogLevel = my::g_LogLevel;
  my::g_LogLevel = LCRITICAL;
  MY_SCOPE_GUARD(restoreLogLevel, [&oldLogLevel]()
                 {
    my::g_LogLevel = oldLogLevel;
  });

  ScopedDir testDir("101010");

  CountryFile germanyFile("Germany");
  LocalCountryFile germanyLocalFile(testDir.GetFullPath(), germanyFile, 101010 /* version */);

  TEST(CountryIndexes::PreparePlaceOnDisk(germanyLocalFile),
       ("Can't prepare place for:", germanyLocalFile));
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
}  // namespace platform
