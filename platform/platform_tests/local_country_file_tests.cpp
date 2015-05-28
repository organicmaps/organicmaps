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

using namespace local_country_file_utils;

namespace
{
void CreateTestDir(string const & testDir)
{
  Platform & platform = GetPlatform();
  TEST(!Platform::IsFileExistsByFullPath(testDir),
       ("Please, remove", testDir, "before running the test."));
  TEST_EQUAL(Platform::ERR_OK, platform.MkDir(testDir), ("Can't create directory", testDir));
}

void CreateTestFile(string const & testFile, string const & contents)
{
  TEST(!Platform::IsFileExistsByFullPath(testFile),
       ("Please, remove", testFile, "before running the test."));
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

  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION, localFile.GetPath(TMapOptions::EMapOnly),
             ());
  TEST_EQUAL("/test-dir/TestCountry" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION,
             localFile.GetPath(TMapOptions::ECarRouting), ());

  // Not synced with disk yet.
  TEST_EQUAL(TMapOptions::ENothing, localFile.GetFiles(), ());

  // Any statement is true about elements of an empty set.
  TEST(localFile.OnDisk(TMapOptions::ENothing), ());

  TEST(!localFile.OnDisk(TMapOptions::EMapOnly), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  TEST_EQUAL("/test-dir", localFile.GetDirectory(), ());

  TEST_EQUAL(0, localFile.GetSize(TMapOptions::ENothing), ());
  TEST_EQUAL(0, localFile.GetSize(TMapOptions::EMapOnly), ());
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

  string const testMapFile = my::JoinFoldersToPath(
      platform.WritableDir(), countryFile.GetNameWithExt(TMapOptions::EMapOnly));
  string const testRoutingFile = my::JoinFoldersToPath(
      platform.WritableDir(), countryFile.GetNameWithExt(TMapOptions::ECarRouting));

  LocalCountryFile localFile(platform.WritableDir(), countryFile, 0 /* version */);
  TEST(!localFile.OnDisk(TMapOptions::EMapOnly), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  CreateTestFile(testMapFile, "map");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMapOnly), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMapOnly), ());

  CreateTestFile(testRoutingFile, "routing");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMapOnly), ());
  TEST(localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMapOnly), ());
  TEST_EQUAL(7, localFile.GetSize(TMapOptions::ECarRouting), ());
  TEST_EQUAL(10, localFile.GetSize(TMapOptions::EMapWithCarRouting), ());

  localFile.DeleteFromDisk();
  TEST(!platform.IsFileExistsByFullPath(testMapFile),
       ("Map file", testMapFile, "wasn't deleted by LocalCountryFile."));
  TEST(!platform.IsFileExistsByFullPath(testRoutingFile),
       ("Routing file", testRoutingFile, "wasn't deleted by LocalCountryFile."));
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

  string const testDir = my::JoinFoldersToPath(platform.WritableDir(), "test-dir");
  CreateTestDir(testDir);
  MY_SCOPE_GUARD(removeTestDir, bind(&Platform::RmDir, testDir));

  string const testIrelandMapFile =
      my::JoinFoldersToPath(testDir, irelandFile.GetNameWithExt(TMapOptions::EMapOnly));
  CreateTestFile(testIrelandMapFile, "Ireland-map");
  MY_SCOPE_GUARD(removeTestIrelandMapFile, bind(&FileWriter::DeleteFileX, testIrelandMapFile));

  string const testNetherlandsMapFile =
      my::JoinFoldersToPath(testDir, netherlandsFile.GetNameWithExt(TMapOptions::EMapOnly));
  CreateTestFile(testNetherlandsMapFile, "Netherlands-map");
  MY_SCOPE_GUARD(removeTestNetherlandsMapFile,
                 bind(&FileWriter::DeleteFileX, testNetherlandsMapFile));

  string const testNetherlandsRoutingFile =
      my::JoinFoldersToPath(testDir, netherlandsFile.GetNameWithExt(TMapOptions::ECarRouting));
  CreateTestFile(testNetherlandsRoutingFile, "Netherlands-routing");
  MY_SCOPE_GUARD(removeTestNetherlandsRoutingFile,
                 bind(&FileWriter::DeleteFileX, testNetherlandsRoutingFile));

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsInDirectory(testDir, 150309, localFiles);
  sort(localFiles.begin(), localFiles.end());
  for (LocalCountryFile & localFile : localFiles)
    localFile.SyncWithDisk();

  LocalCountryFile expectedIrelandFile(testDir, irelandFile, 150309);
  expectedIrelandFile.m_files = TMapOptions::EMapOnly;

  LocalCountryFile expectedNetherlandsFile(testDir, netherlandsFile, 150309);
  expectedNetherlandsFile.m_files = TMapOptions::EMapWithCarRouting;

  vector<LocalCountryFile> expectedLocalFiles = {expectedIrelandFile, expectedNetherlandsFile};

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

  string const testDir = my::JoinFoldersToPath(platform.WritableDir(), "010101");
  CreateTestDir(testDir);
  MY_SCOPE_GUARD(removeTestDir, bind(&Platform::RmDir, testDir));

  string const testItalyMapFile =
      my::JoinFoldersToPath(testDir, italyFile.GetNameWithExt(TMapOptions::EMapOnly));
  CreateTestFile(testItalyMapFile, "Italy-map");
  MY_SCOPE_GUARD(remoteTestItalyMapFile, bind(&FileWriter::DeleteFileX, testItalyMapFile));

  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  LOG(LINFO, (localFiles));
  multiset<LocalCountryFile> localFilesSet(localFiles.begin(), localFiles.end());

  LocalCountryFile expectedWorldFile(platform.WritableDir(), CountryFile(WORLD_FILE_NAME),
                                     0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldFile), ());

  LocalCountryFile expectedWorldCoastsFile(platform.WritableDir(),
                                           CountryFile(WORLD_COASTS_FILE_NAME), 0 /* version */);
  TEST_EQUAL(1, localFilesSet.count(expectedWorldCoastsFile), ());

  LocalCountryFile expectedItalyFile(testDir, italyFile, 10101);
  TEST_EQUAL(1, localFilesSet.count(expectedItalyFile), ());
}
