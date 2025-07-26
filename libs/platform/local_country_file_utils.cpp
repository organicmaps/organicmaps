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

namespace platform
{
using std::string;

namespace
{
char const kBitsExt[] = ".bftsegbits";
char const kNodesExt[] = ".bftsegnodes";
char const kOffsetsExt[] = ".offsets";

string GetAdditionalWorldScope()
{
  return "r";
}
/*
bool IsSpecialName(string const & name) { return name == "." || name == ".."; }
*/
bool IsDownloaderFile(string const & name)
{
  static std::regex const filter(".*\\.(downloading|resume|ready)[0-9]?$");
  return std::regex_match(name.begin(), name.end(), filter);
}

bool IsDiffFile(string const & name)
{
  return name.ends_with(DIFF_FILE_EXTENSION) || name.ends_with(DIFF_APPLYING_FILE_EXTENSION);
}

/*
bool DirectoryHasIndexesOnly(string const & directory)
{
  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(directory, Platform::EFileType::Regular | Platform::EFileType::Directory, fwts);

  for (auto const & fwt : fwts)
  {
    auto const & name = fwt.first;
    auto const & type = fwt.second;
    if (type == Platform::EFileType::Directory)
    {
      if (!IsSpecialName(name))
        return false;
    }
    else if (!CountryIndexes::IsIndexFile(name))
      return false;
  }

  return true;
}
*/

inline string GetDataDirFullPath(string const & dataDir)
{
  Platform & platform = GetPlatform();
  return dataDir.empty() ? platform.WritableDir() : base::JoinPath(platform.WritableDir(), dataDir);
}

void FindAllDiffsInDirectory(string const & dir, std::vector<LocalCountryFile> & diffs)
{
  Platform & platform = GetPlatform();

  Platform::TFilesWithType files;
  platform.GetFilesByType(dir, Platform::EFileType::Regular, files);

  for (auto const & fileWithType : files)
  {
    string name = fileWithType.first;

    auto const isDiffReady = name.ends_with(DIFF_FILE_EXTENSION READY_FILE_EXTENSION);
    auto const isDiff = name.ends_with(DIFF_FILE_EXTENSION);

    if (!isDiff && !isDiffReady)
      continue;

    base::GetNameWithoutExt(name);
    if (isDiffReady)
      base::GetNameWithoutExt(name);

    diffs.emplace_back(dir, CountryFile(std::move(name)), 0 /* version */);
  }
}
}  // namespace

string GetFilePath(int64_t version, string const & dataDir, string const & countryName, MapFileType type)
{
  string const filename = GetFileName(countryName, type);
  string const dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return base::JoinPath(dir, filename);
  return base::JoinPath(dir, strings::to_string(version), filename);
}

void DeleteDownloaderFilesForCountry(int64_t version, CountryFile const & countryFile)
{
  DeleteDownloaderFilesForCountry(version, string(), countryFile);
}

void DeleteDownloaderFilesForCountry(int64_t version, string const & dataDir, CountryFile const & countryFile)
{
  for (size_t type = 0; type < base::Underlying(MapFileType::Count); ++type)
  {
    string const path = GetFileDownloadPath(version, dataDir, countryFile, static_cast<MapFileType>(type));
    ASSERT(path.ends_with(READY_FILE_EXTENSION), ());
    Platform::RemoveFileIfExists(path);
    Platform::RemoveFileIfExists(path + RESUME_FILE_EXTENSION);
    Platform::RemoveFileIfExists(path + DOWNLOADING_FILE_EXTENSION);
  }

  // Delete the diff that was downloaded but wasn't applied.
  {
    string const path = GetFilePath(version, dataDir, countryFile.GetName(), MapFileType::Diff);
    Platform::RemoveFileIfExists(path);
  }
}

size_t FindAllLocalMapsInDirectoryAndCleanup(string const & directory, int64_t version, int64_t latestVersion,
                                             std::vector<LocalCountryFile> & localFiles)
{
  Platform & platform = GetPlatform();
  size_t const szBefore = localFiles.size();

  Platform::TFilesWithType fwts;
  platform.GetFilesByType(directory, Platform::EFileType::Regular | Platform::EFileType::Directory, fwts);

  for (auto const & fwt : fwts)
  {
    if (fwt.second != Platform::EFileType::Regular)
      continue;

    string name = fwt.first;

    // Remove downloader and diff files for old version directories.
    if (version < latestVersion && (IsDownloaderFile(name) || IsDiffFile(name)))
    {
      base::DeleteFileX(base::JoinPath(directory, name));
      continue;
    }

    if (!name.ends_with(DATA_FILE_EXTENSION))
      continue;

    // Remove DATA_FILE_EXTENSION and use base name as a country file name.
    base::GetNameWithoutExt(name);
    localFiles.emplace_back(directory, CountryFile(std::move(name)), version);
  }

  return localFiles.size() - szBefore;

  // Probably, indices will appear in future.
  /*
  for (auto const & fwt : fwts)
  {
    if (fwt.second != Platform::EFileType::Directory)
      continue;

    string const & name = fwt.first;
    if (IsSpecialName(name))
      continue;

    if (names.count(name) == 0 && DirectoryHasIndexesOnly(base::JoinPath(directory, name)))
    {
      // Directory which looks like a directory with indexes for absent country. It's OK to remove it.
      LocalCountryFile absentCountry(directory, CountryFile(name), version);
      CountryIndexes::DeleteFromDisk(absentCountry);
    }
  }
  */
}

void FindAllDiffs(std::string const & dataDir, std::vector<LocalCountryFile> & diffs)
{
  string const dir = GetDataDirFullPath(dataDir);
  FindAllDiffsInDirectory(dir, diffs);

  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(dir, Platform::EFileType::Directory, fwts);

  for (auto const & fwt : fwts)
    FindAllDiffsInDirectory(base::JoinPath(dir, fwt.first /* subdir */), diffs);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, std::vector<LocalCountryFile> & localFiles)
{
  FindAllLocalMapsAndCleanup(latestVersion, string(), localFiles);
}

void FindAllLocalMapsAndCleanup(int64_t latestVersion, string const & dataDir,
                                std::vector<LocalCountryFile> & localFiles)
{
  string const dir = GetDataDirFullPath(dataDir);

  // Do not search root folder! We have separate World processing in Storage::GetForceDownloadWorlds.
  // FindAllLocalMapsInDirectoryAndCleanup(dir, 0 /* version */, latestVersion, localFiles);

  Platform::TFilesWithType fwts;
  Platform::GetFilesByType(dir, Platform::EFileType::Directory, fwts);
  for (auto const & fwt : fwts)
  {
    string const & subdir = fwt.first;
    int64_t version;
    if (!ParseVersion(subdir, version) || version > latestVersion)
      continue;

    string const fullPath = base::JoinPath(dir, subdir);
    if (0 == FindAllLocalMapsInDirectoryAndCleanup(fullPath, version, latestVersion, localFiles))
    {
      Platform::EError err = Platform::RmDir(fullPath);
      if (err != Platform::ERR_OK)
        LOG(LWARNING, ("Can't remove directory:", fullPath, err));
    }
  }

  // Check for World and WorldCoasts in app bundle or in resources.
  Platform & platform = GetPlatform();
  string const world(WORLD_FILE_NAME);
  string const worldCoasts(WORLD_COASTS_FILE_NAME);
  for (string const & file : {world, worldCoasts})
  {
    auto i = localFiles.begin();
    for (; i != localFiles.end(); ++i)
      if (i->GetCountryName() == file)
        break;

    try
    {
      ModelReaderPtr reader(platform.GetReader(file + DATA_FILE_EXTENSION, GetAdditionalWorldScope()));

      // Empty path means the resource file.
      LocalCountryFile worldFile(string(), CountryFile(file), version::ReadVersionDate(reader));
      worldFile.m_files[base::Underlying(MapFileType::Map)] = reader.Size();

      // Replace if newer only.
      if (i != localFiles.end())
      {
        if (worldFile.GetVersion() > i->GetVersion())
          *i = std::move(worldFile);
      }
      else
        localFiles.push_back(std::move(worldFile));
    }
    catch (RootException const & ex)
    {
      if (i == localFiles.end())
      {
        // This warning is possible on android devices without bundled Worlds.
        LOG(LWARNING, ("Can't find any:", file, "Reason:", ex.Msg()));
      }
    }
  }
}

void CleanupMapsDirectory(int64_t latestVersion)
{
  std::vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(latestVersion, localFiles);
}

bool ParseVersion(string const & s, int64_t & version)
{
  // Folder version format is 211122. Unit tests use simple "1", "2" versions.
  if (s.empty() || s.size() > 6)
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

std::shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, CountryFile const & countryFile)
{
  return PreparePlaceForCountryFiles(version, string(), countryFile);
}

std::shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, string const & dataDir,
                                                              CountryFile const & countryFile)
{
  string const dir = PrepareDirToDownloadCountry(version, dataDir);
  return (!dir.empty() ? make_shared<LocalCountryFile>(dir, countryFile, version) : nullptr);
}

std::string PrepareDirToDownloadCountry(int64_t version, std::string const & dataDir)
{
  string dir = GetDataDirFullPath(dataDir);
  if (version == 0)
    return dir;
  dir = base::JoinPath(dir, strings::to_string(version));
  return (Platform::MkDirChecked(dir) ? dir : std::string());
}

string GetFileDownloadPath(int64_t version, string const & dataDir, string const & countryName, MapFileType type)
{
  return GetFilePath(version, dataDir, countryName, type) + READY_FILE_EXTENSION;
}

std::unique_ptr<ModelReader> GetCountryReader(LocalCountryFile const & file, MapFileType type)
{
  Platform & platform = GetPlatform();
  if (file.IsInBundle())
    return platform.GetReader(file.GetFileName(type), GetAdditionalWorldScope());
  else
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
void CountryIndexes::GetIndexesExts(std::vector<string> & exts)
{
  exts.push_back(kBitsExt);
  exts.push_back(kNodesExt);
  exts.push_back(kOffsetsExt);
}

// static
bool CountryIndexes::IsIndexFile(string const & file)
{
  return file.ends_with(kBitsExt) || file.ends_with(kNodesExt) || file.ends_with(kOffsetsExt);
}

// static
string CountryIndexes::IndexesDir(LocalCountryFile const & localFile)
{
  string dir = localFile.GetDirectory();
  CountryFile const & file = localFile.GetCountryFile();

  if (localFile.IsInBundle())
  {
    // Local file is stored in bundle. Need to prepare index folder in the writable directory.
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
