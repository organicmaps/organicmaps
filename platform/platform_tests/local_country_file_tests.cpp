#include "testing/testing.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

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

// Scoped test directory in a writable dir.
class ScopedTestDir
{
public:
  /// Creates test dir in a writable directory.
  /// @param path Path for a testing directory, should be relative to writable-dir.
  ScopedTestDir(string const & relativePath)
      : m_fullPath(my::JoinFoldersToPath(GetPlatform().WritableDir(), relativePath)),
        m_relativePath(relativePath),
        m_reset(false)
  {
    Platform & platform = GetPlatform();
    Platform::EError ret = platform.MkDir(GetFullPath());
    switch (ret)
    {
      case Platform::ERR_OK:
        break;
      case Platform::ERR_FILE_ALREADY_EXISTS:
        Platform::EFileType type;
        TEST_EQUAL(Platform::ERR_OK, Platform::GetFileType(GetFullPath(), type), ());
        TEST_EQUAL(Platform::FILE_TYPE_DIRECTORY, type, ());
        break;
      default:
        CHECK(false, ("Can't create directory:", GetFullPath(), "error:", ret));
        break;
    }
  }

  ~ScopedTestDir()
  {
    if (m_reset)
      return;

    string const fullPath = GetFullPath();
    Platform::EError ret = Platform::RmDir(fullPath);
    switch (ret)
    {
      case Platform::ERR_OK:
        break;
      case Platform::ERR_FILE_DOES_NOT_EXIST:
        LOG(LWARNING, (fullPath, "was deleted before destruction of ScopedTestDir."));
        break;
      case Platform::ERR_DIRECTORY_NOT_EMPTY:
        LOG(LWARNING, ("There are files in", fullPath));
        break;
      default:
        LOG(LWARNING, ("Platform::RmDir() error for", fullPath, ":", ret));
        break;
    }
  }

  inline void Reset() { m_reset = true; }

  inline string const & GetFullPath() const { return m_fullPath; }

  inline string const & GetRelativePath() const { return m_relativePath; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  string const m_relativePath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedTestDir);
};

class ScopedTestFile
{
public:
  ScopedTestFile(string const & relativePath, string const & contents)
      : m_fullPath(my::JoinFoldersToPath(GetPlatform().WritableDir(), relativePath)), m_reset(false)
  {
    {
      FileWriter writer(GetFullPath());
      writer.Write(contents.data(), contents.size());
    }
    TEST(Exists(), ("Can't create test file", GetFullPath()));
  }

  ScopedTestFile(ScopedTestDir const & dir, CountryFile const & countryFile, TMapOptions file,
                 string const & contents)
      : ScopedTestFile(
            my::JoinFoldersToPath(dir.GetRelativePath(), countryFile.GetNameWithExt(file)),
            contents)
  {
  }

  ~ScopedTestFile()
  {
    if (m_reset)
      return;
    if (!Exists())
    {
      LOG(LWARNING, ("File", GetFullPath(), "was deleted before dtor of ScopedTestFile."));
      return;
    }
    if (!my::DeleteFileX(GetFullPath()))
      LOG(LWARNING, ("Can't remove test file:", GetFullPath()));
  }

  inline string const & GetFullPath() const { return m_fullPath; }

  inline void Reset() { m_reset = true; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedTestFile);
};

string DebugPrint(ScopedTestDir const & dir)
{
  ostringstream os;
  os << "ScopedTestDir [" << dir.GetFullPath() << "]";
  return os.str();
}

string DebugPrint(ScopedTestFile const & file)
{
  ostringstream os;
  os << "ScopedTestFile [" << file.GetFullPath() << "]";
  return os.str();
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

  ScopedTestFile testMapFile(countryFile.GetNameWithExt(TMapOptions::EMap), "map");

  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMap), ());
  TEST(!localFile.OnDisk(TMapOptions::ECarRouting), ());
  TEST(!localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());
  TEST_EQUAL(3, localFile.GetSize(TMapOptions::EMap), ());

  ScopedTestFile testRoutingFile(countryFile.GetNameWithExt(TMapOptions::ECarRouting), "routing");

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

  ScopedTestDir testDir1("1");
  LocalCountryFile japanLocalFile(testDir1.GetFullPath(), japanFile, 1 /* version */);
  ScopedTestFile japanMapFile(testDir1, japanFile, TMapOptions::EMap, "Japan");

  ScopedTestDir testDir2("2");
  LocalCountryFile brazilLocalFile(testDir2.GetFullPath(), brazilFile, 2 /* version */);
  ScopedTestFile brazilMapFile(testDir2, brazilFile, TMapOptions::EMap, "Brazil");
  LocalCountryFile irelandLocalFile(testDir2.GetFullPath(), irelandFile, 2 /* version */);
  ScopedTestFile irelandMapFile(testDir2, irelandFile, TMapOptions::EMap, "Ireland");

  ScopedTestDir testDir3("3");

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
  ScopedTestFile toBeDeleted[] = {{"Ireland.mwm.ready", "Ireland"},
                                  {"Netherlands.mwm.routing.downloading2", "Netherlands"},
                                  {"Germany.mwm.ready3", "Germany"},
                                  {"UK_England.mwm.resume4", "UK"}};
  ScopedTestFile toBeKept[] = {
      {"Italy.mwm", "Italy"}, {"Spain.mwm", "Spain map"}, {"Spain.mwm.routing", "Spain routing"}};

  CleanupMapsDirectory();

  for (ScopedTestFile & file : toBeDeleted)
  {
    TEST(!file.Exists(), (file));
    file.Reset();
  }

  for (ScopedTestFile & file : toBeKept)
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

  ScopedTestDir testDir("test-dir");

  ScopedTestFile testIrelandMapFile(testDir, irelandFile, TMapOptions::EMap, "Ireland-map");
  ScopedTestFile testNetherlandsMapFile(testDir, netherlandsFile, TMapOptions::EMap,
                                        "Netherlands-map");
  ScopedTestFile testNetherlandsRoutingFile(testDir, netherlandsFile, TMapOptions::ECarRouting,
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

  ScopedTestDir testDir("010101");
  ScopedTestFile testItalyMapFile(testDir, italyFile, TMapOptions::EMap, "Italy-map");

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

  ScopedTestDir directoryForV1("1");

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
}  // namespace platform
