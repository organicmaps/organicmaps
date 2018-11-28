#include "platform/local_country_file_utils.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/cctype.hpp"
#include "std/regex.hpp"
#include "std/sstream.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"

#include "defines.hpp"

namespace platform
{

namespace migrate
{
// Set of functions to support migration between different versions of MWM
// with totaly incompatible formats.
// 160302 - Migrate to small single file MWM
uint32_t constexpr kMinRequiredVersion = 160302;
bool NeedMigrate()
{
  uint32_t version;
  if (!settings::Get("LastMigration", version))
    return true;

  if (version >= kMinRequiredVersion)
    return false;

  return true;
}

void SetMigrationFlag()
{
  settings::Set("LastMigration", kMinRequiredVersion);
}
}  // namespace migrate

namespace
{
char const kBitsExt[] = ".bftsegbits";
char const kNodesExt[] = ".bftsegnodes";
char const kOffsetsExt[] = ".offsets";

size_t const kMaxTimestampLength = 18;

string GetSpecialFilesSearchScope()
{
#if defined(OMIM_OS_ANDROID)
  return "er";
#else
  return "r";
#endif  // defined(OMIM_OS_ANDROID)
}

bool IsSpecialName(string const & name) { return name == "." || name == ".."; }

bool IsDownloaderFile(string const & name)
{
  static regex const filter(".*\\.(downloading|resume|ready)[0-9]?$");
  return regex_match(name.begin(), name.end(), filter);
}

bool IsDiffFile(string const & name)
{
  return strings::EndsWith(name, DIFF_FILE_EXTENSION) ||
         strings::EndsWith(name, DIFF_APPLYING_FILE_EXTENSION);
}

bool DirectoryHasIndexesOnly(string const & directory)
{
  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(directory, Platform::FILE_TYPE_REGULAR | Platform::FILE_TYPE_DIRECTORY,
                           fwts);
  for (auto const & fwt : fwts)
  {
    auto const & name = fwt.first;
    auto const & type = fwt.second;
    if (type == Platform::FILE_TYPE_DIRECTORY)
    {
      if (!IsSpecialName(name))
        return false;
      continue;
    }
    if (!CountryIndexes::IsIndexFile(name))
      return false;
  }
  return true;
}

inline string GetDataDirFullPath(string const & dataDir)
{
  Platform & platform = GetPlatform();
  return dataDir.empty() ? platform.WritableDir()
                         : base::JoinFoldersToPath(platform.WritableDir(), dataDir);
}

void FindAllDiffsInDirectory(string const & dir, vector<LocalCountryFile> & diffs)
{
  Platform & platform = GetPlatform();

  Platform::TFilesWithType files;
  platform.GetFilesByType(dir, Platform::FILE_TYPE_REGULAR, files);

  for (auto const & fileWithType : files)
  {
    string name = fileWithType.first;

    auto const isDiffReady =
        strings::EndsWith(name, strings::to_string(DIFF_FILE_EXTENSION) + READY_FILE_EXTENSION);
    auto const isDiff = strings::EndsWith(name, DIFF_FILE_EXTENSION);

    if (!isDiff && !isDiffReady)
      continue;

    base::GetNameWithoutExt(name);

    if (isDiffReady)
      base::GetNameWithoutExt(name);

    LocalCountryFile localDiff(dir, CountryFile(name), 0 /* version */);

    diffs.push_back(localDiff);
  }
}
}  // namespace

void DeleteDownloaderFilesForCountry(int64_t version, CountryFile const & countryFile)
{
  DeleteDownloaderFilesForCountry(version, string(), countryFile);
}

void DeleteDownloaderFilesForCountry(int64_t version, string const & dataDir,
                                     CountryFile const & countryFile)
{
  for (MapOptions file : {MapOptions::Map, MapOptions::CarRouting, MapOptions::Diff})
  {
    string const path = GetFileDownloadPath(version, dataDir, countryFile, file);
    ASSERT(strings::EndsWith(path, READY_FILE_EXTENSION), ());
    base::DeleteFileX(path);
    base::DeleteFileX(path + RESUME_FILE_EXTENSION);
    base::DeleteFileX(path + DOWNLOADING_FILE_EXTENSION);
  }
}

void FindAllLocalMapsInDirectoryAndCleanup(string const & directory, int64_t version,
                                           int64_t latestVersion,
                                           vector<LocalCountryFile> & localFiles)
{
  Platform & platform = GetPlatform();

  Platform::TFilesWithType fwts;
  platform.GetFilesByType(directory, Platform::FILE_TYPE_REGULAR | Platform::FILE_TYPE_DIRECTORY,
                          fwts);

  unordered_set<string> names;
  for (auto const & fwt : fwts)
  {
    if (fwt.second != Platform::FILE_TYPE_REGULAR)
      continue;

    string name = fwt.first;

    // Remove downloader and diff files for old version directories.
    if (version < latestVersion && (IsDownloaderFile(name) || IsDiffFile(name)))
    {
      base::DeleteFileX(base::JoinFoldersToPath(directory, name));
      continue;
    }

    if (!strings::EndsWith(name, DATA_FILE_EXTENSION))
      continue;

    // Remove DATA_FILE_EXTENSION and use base name as a country file name.
    base::GetNameWithoutExt(name);
    names.insert(name);
    LocalCountryFile localFile(directory, CountryFile(name), version);

    // Delete Brazil.mwm and Japan.mwm maps, because they were
    // replaced with smaller regions after osrm routing
    // implementation.
    if (name == "Japan" || name == "Brazil")
    {
      localFile.SyncWithDisk();
      localFile.DeleteFromDisk(MapOptions::MapWithCarRouting);
      continue;
    }

    localFiles.push_back(localFile);
  }

  for (auto const & fwt : fwts)
  {
    if (fwt.second != Platform::FILE_TYPE_DIRECTORY)
      continue;

    string name = fwt.first;
    if (IsSpecialName(name))
      continue;

    if (names.count(name) == 0 && DirectoryHasIndexesOnly(base::JoinFoldersToPath(directory, name)))
    {
      // Directory which looks like a directory with indexes for absent country. It's OK to remove
      // it.
      LocalCountryFile absentCountry(directory, CountryFile(name), version);
      CountryIndexes::DeleteFromDisk(absentCountry);
    }
  }
}

void FindAllDiffs(string const & dataDir, vector<LocalCountryFile> & diffs)
{
  string const dir = GetDataDirFullPath(dataDir);
  FindAllDiffsInDirectory(dir, diffs);

  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(dir, Platform::FILE_TYPE_DIRECTORY, fwts);

  for (auto const & fwt : fwts)
    FindAllDiffsInDirectory(base::JoinFoldersToPath(dir, fwt.first /* subdir */), diffs);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, vector<LocalCountryFile> & localFiles)
{
  FindAllLocalMapsAndCleanup(latestVersion, string(), localFiles);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, string const & dataDir,
                                vector<LocalCountryFile> & localFiles)
{
  string const dir = GetDataDirFullPath(dataDir);
  FindAllLocalMapsInDirectoryAndCleanup(dir, 0 /* version */, latestVersion, localFiles);

  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(dir, Platform::FILE_TYPE_DIRECTORY, fwts);
  for (auto const & fwt : fwts)
  {
    string const & subdir = fwt.first;
    int64_t version;
    if (!ParseVersion(subdir, version) || version > latestVersion)
      continue;

    string const fullPath = base::JoinFoldersToPath(dir, subdir);
    FindAllLocalMapsInDirectoryAndCleanup(fullPath, version, latestVersion, localFiles);
    Platform::EError err = Platform::RmDir(fullPath);
    if (err != Platform::ERR_OK && err != Platform::ERR_DIRECTORY_NOT_EMPTY)
      LOG(LWARNING, ("Can't remove directory:", fullPath, err));
  }

  // World and WorldCoasts can be stored in app bundle or in resources
  // directory, thus it's better to get them via Platform.
  for (string const & file : { WORLD_FILE_NAME,
    (migrate::NeedMigrate() ? WORLD_COASTS_OBSOLETE_FILE_NAME : WORLD_COASTS_FILE_NAME) })
  {
    auto i = localFiles.begin();
    for (; i != localFiles.end(); ++i)
    {
      if (i->GetCountryFile().GetName() == file)
        break;
    }

    try
    {
      Platform & platform = GetPlatform();
      ModelReaderPtr reader(
          platform.GetReader(file + DATA_FILE_EXTENSION, GetSpecialFilesSearchScope()));

      // Assume that empty path means the resource file.
      LocalCountryFile worldFile{string(), CountryFile(file),
                                 version::ReadVersionDate(reader)};
      worldFile.m_files = MapOptions::Map;
      if (i != localFiles.end())
      {
        // Always use resource World files instead of local on disk.
        *i = worldFile;
      }
      else
        localFiles.push_back(worldFile);
    }
    catch (RootException const & ex)
    {
      if (i == localFiles.end())
      {
        // This warning is possible on android devices without pre-downloaded Worlds/fonts files.
        LOG(LWARNING, ("Can't find any:", file, "Reason:", ex.Msg()));
      }
    }
  }
}

void CleanupMapsDirectory(int64_t latestVersion)
{
  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(latestVersion, localFiles);
}

bool ParseVersion(string const & s, int64_t & version)
{
  if (s.empty() || s.size() > kMaxTimestampLength)
    return false;

  int64_t v = 0;
  for (char const c : s)
  {
    if (!isdigit(c))
      return false;
    v = v * 10 + c - '0';
  }
  version = v;
  return true;
}

shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, CountryFile const & countryFile)
{
  return PreparePlaceForCountryFiles(version, string(), countryFile);
}

shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, string const & dataDir,
                                                         CountryFile const & countryFile)
{
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return make_shared<LocalCountryFile>(dir, countryFile, version);
  string const directory = base::JoinFoldersToPath(dir, strings::to_string(version));
  if (!Platform::MkDirChecked(directory))
    return shared_ptr<LocalCountryFile>();
  return make_shared<LocalCountryFile>(directory, countryFile, version);
}

string GetFileDownloadPath(int64_t version, CountryFile const & countryFile, MapOptions options)
{
  return GetFileDownloadPath(version, string(), countryFile, options);
}

string GetFileDownloadPath(int64_t version, string const & dataDir,
                           CountryFile const & countryFile, MapOptions options)
{
  string const readyFile = GetFileName(countryFile.GetName(), options, version) + READY_FILE_EXTENSION;
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return base::JoinFoldersToPath(dir, readyFile);
  return base::JoinFoldersToPath({dir, strings::to_string(version)}, readyFile);
}

unique_ptr<ModelReader> GetCountryReader(platform::LocalCountryFile const & file, MapOptions options)
{
  Platform & platform = GetPlatform();
  // See LocalCountryFile comment for explanation.
  if (file.GetDirectory().empty())
  {
    return platform.GetReader(file.GetCountryName() + DATA_FILE_EXTENSION,
                              GetSpecialFilesSearchScope());
  }
  return platform.GetReader(file.GetPath(options), "f");
}

// static
void CountryIndexes::PreparePlaceOnDisk(LocalCountryFile const & localFile)
{
  string const dir = IndexesDir(localFile);
  if (!Platform::MkDirChecked(dir))
    MYTHROW(FileSystemException, ("Can't create directory", dir));
}

// static
bool CountryIndexes::DeleteFromDisk(LocalCountryFile const & localFile)
{
  string const directory = IndexesDir(localFile);
  bool ok = true;

  for (auto index : {Index::Bits, Index::Nodes, Index::Offsets})
  {
    string const path = GetPath(localFile, index);
    if (Platform::IsFileExistsByFullPath(path) && !base::DeleteFileX(path))
    {
      LOG(LWARNING, ("Can't remove country index:", path));
      ok = false;
    }
  }

  Platform::EError const ret = Platform::RmDir(directory);
  if (ret != Platform::ERR_OK && ret != Platform::ERR_FILE_DOES_NOT_EXIST)
  {
    LOG(LWARNING, ("Can't remove indexes directory:", directory, ret));
    ok = false;
  }
  return ok;
}

// static
string CountryIndexes::GetPath(LocalCountryFile const & localFile, Index index)
{
  char const * ext = nullptr;
  switch (index)
  {
    case Index::Bits:
      ext = kBitsExt;
      break;
    case Index::Nodes:
      ext = kNodesExt;
      break;
    case Index::Offsets:
      ext = kOffsetsExt;
      break;
  }
  return base::JoinFoldersToPath(IndexesDir(localFile), localFile.GetCountryName() + ext);
}

// static
void CountryIndexes::GetIndexesExts(vector<string> & exts)
{
  exts.push_back(kBitsExt);
  exts.push_back(kNodesExt);
  exts.push_back(kOffsetsExt);
}

// static
bool CountryIndexes::IsIndexFile(string const & file)
{
  return strings::EndsWith(file, kBitsExt) || strings::EndsWith(file, kNodesExt) ||
         strings::EndsWith(file, kOffsetsExt);
}

// static
string CountryIndexes::IndexesDir(LocalCountryFile const & localFile)
{
  string dir = localFile.GetDirectory();
  CountryFile const & file = localFile.GetCountryFile();

  /// @todo It's a temporary code until we will put fIndex->fOffset into mwm container.
  /// IndexesDir should not throw any exceptions.
  if (dir.empty())
  {
    // Local file is stored in resources. Need to prepare index folder in the writable directory.
    int64_t const version = localFile.GetVersion();
    ASSERT_GREATER(version, 0, ());

    dir = base::JoinFoldersToPath(GetPlatform().WritableDir(), strings::to_string(version));
    if (!Platform::MkDirChecked(dir))
      MYTHROW(FileSystemException, ("Can't create directory", dir));
  }

  return base::JoinFoldersToPath(dir, file.GetName());
}

string DebugPrint(CountryIndexes::Index index)
{
  switch (index)
  {
    case CountryIndexes::Index::Bits:
      return "Bits";
    case CountryIndexes::Index::Nodes:
      return "Nodes";
    case CountryIndexes::Index::Offsets:
      return "Offsets";
  }
  UNREACHABLE();
}
}  // namespace platform
