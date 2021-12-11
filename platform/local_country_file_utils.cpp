#include "platform/local_country_file_utils.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <regex>
#include <sstream>
#include <unordered_set>

#include "defines.hpp"

using namespace std;

namespace platform
{
namespace
{
char const kBitsExt[] = ".bftsegbits";
char const kNodesExt[] = ".bftsegnodes";
char const kOffsetsExt[] = ".offsets";

size_t const kMaxTimestampLength = 18;

string GetSpecialFilesSearchScope()
{
  return "r";
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
  return dataDir.empty() ? platform.WritableDir() : base::JoinPath(platform.WritableDir(), dataDir);
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

string GetFilePath(int64_t version, string const & dataDir, CountryFile const & countryFile,
                   MapFileType type)
{
  string const filename = GetFileName(countryFile.GetName(), type);
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return base::JoinPath(dir, filename);
  return base::JoinPath(dir, strings::to_string(version), filename);
}
}  // namespace

void DeleteDownloaderFilesForCountry(int64_t version, CountryFile const & countryFile)
{
  DeleteDownloaderFilesForCountry(version, string(), countryFile);
}

void DeleteDownloaderFilesForCountry(int64_t version, string const & dataDir,
                                     CountryFile const & countryFile)
{
  for (size_t type = 0; type < base::Underlying(MapFileType::Count); ++type)
  {
    string const path = GetFileDownloadPath(version, dataDir, countryFile,
                                            static_cast<MapFileType>(type));
    ASSERT(strings::EndsWith(path, READY_FILE_EXTENSION), ());
    Platform::RemoveFileIfExists(path);
    Platform::RemoveFileIfExists(path + RESUME_FILE_EXTENSION);
    Platform::RemoveFileIfExists(path + DOWNLOADING_FILE_EXTENSION);
  }

  // Delete the diff that was downloaded but wasn't applied.
  {
    string const path = GetFilePath(version, dataDir, countryFile, MapFileType::Diff);
    Platform::RemoveFileIfExists(path);
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
      base::DeleteFileX(base::JoinPath(directory, name));
      continue;
    }

    if (!strings::EndsWith(name, DATA_FILE_EXTENSION))
      continue;

    // Remove DATA_FILE_EXTENSION and use base name as a country file name.
    base::GetNameWithoutExt(name);
    names.insert(name);
    LocalCountryFile localFile(directory, CountryFile(name), version);

    localFiles.push_back(localFile);
  }

  for (auto const & fwt : fwts)
  {
    if (fwt.second != Platform::FILE_TYPE_DIRECTORY)
      continue;

    string name = fwt.first;
    if (IsSpecialName(name))
      continue;

    if (names.count(name) == 0 && DirectoryHasIndexesOnly(base::JoinPath(directory, name)))
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
    FindAllDiffsInDirectory(base::JoinPath(dir, fwt.first /* subdir */), diffs);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, vector<LocalCountryFile> & localFiles)
{
  FindAllLocalMapsAndCleanup(latestVersion, string(), localFiles);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, string const & dataDir,
                                vector<LocalCountryFile> & localFiles)
{
  string const dir = GetDataDirFullPath(dataDir);

  /// @todo Should we search for files in root data folder, except minsk-pass tests?
  FindAllLocalMapsInDirectoryAndCleanup(dir, 0 /* version */, latestVersion, localFiles);

  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(dir, Platform::FILE_TYPE_DIRECTORY, fwts);
  for (auto const & fwt : fwts)
  {
    string const & subdir = fwt.first;
    int64_t version;
    if (!ParseVersion(subdir, version) || version > latestVersion)
      continue;

    string const fullPath = base::JoinPath(dir, subdir);
    FindAllLocalMapsInDirectoryAndCleanup(fullPath, version, latestVersion, localFiles);
    Platform::EError err = Platform::RmDir(fullPath);
    if (err != Platform::ERR_OK && err != Platform::ERR_DIRECTORY_NOT_EMPTY)
      LOG(LWARNING, ("Can't remove directory:", fullPath, err));
  }

  // World and WorldCoasts can be stored in app bundle or in resources
  // directory, thus it's better to get them via Platform.
  string const world(WORLD_FILE_NAME);
  string const worldCoasts(WORLD_COASTS_FILE_NAME);
  for (string const & file : {world, worldCoasts})
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
      LocalCountryFile worldFile{string(), CountryFile(file), version::ReadVersionDate(reader)};
      worldFile.m_files[base::Underlying(MapFileType::Map)] = 1;
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

shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version,
                                                         CountryFile const & countryFile)
{
  return PreparePlaceForCountryFiles(version, string(), countryFile);
}

shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, string const & dataDir,
                                                         CountryFile const & countryFile)
{
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return make_shared<LocalCountryFile>(dir, countryFile, version);
  string const directory = base::JoinPath(dir, strings::to_string(version));
  if (!Platform::MkDirChecked(directory))
    return shared_ptr<LocalCountryFile>();
  return make_shared<LocalCountryFile>(directory, countryFile, version);
}

string GetFileDownloadPath(int64_t version, CountryFile const & countryFile, MapFileType type)
{
  return GetFileDownloadPath(version, string(), countryFile, type);
}

string GetFileDownloadPath(int64_t version, string const & dataDir, CountryFile const & countryFile,
                           MapFileType type)
{
  string const readyFile = GetFileName(countryFile.GetName(), type) + READY_FILE_EXTENSION;
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return base::JoinPath(dir, readyFile);
  return base::JoinPath(dir, strings::to_string(version), readyFile);
}

unique_ptr<ModelReader> GetCountryReader(platform::LocalCountryFile const & file, MapFileType type)
{
  Platform & platform = GetPlatform();
  // See LocalCountryFile comment for explanation.
  if (file.GetDirectory().empty())
  {
    return platform.GetReader(file.GetCountryName() + DATA_FILE_EXTENSION,
                              GetSpecialFilesSearchScope());
  }
  return platform.GetReader(file.GetPath(type), "f");
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
  case Index::Bits: ext = kBitsExt; break;
  case Index::Nodes: ext = kNodesExt; break;
  case Index::Offsets: ext = kOffsetsExt; break;
  }
  return base::JoinPath(IndexesDir(localFile), localFile.GetCountryName() + ext);
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

    dir = base::JoinPath(GetPlatform().WritableDir(), strings::to_string(version));
    if (!Platform::MkDirChecked(dir))
      MYTHROW(FileSystemException, ("Can't create directory", dir));
  }

  return base::JoinPath(dir, file.GetName());
}

string DebugPrint(CountryIndexes::Index index)
{
  switch (index)
  {
  case CountryIndexes::Index::Bits: return "Bits";
  case CountryIndexes::Index::Nodes: return "Nodes";
  case CountryIndexes::Index::Offsets: return "Offsets";
  }
  UNREACHABLE();
}
}  // namespace platform
