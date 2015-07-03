#include "platform/local_country_file_utils.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/cctype.hpp"
#include "std/sstream.hpp"

namespace platform
{
namespace
{
size_t const kMaxTimestampLength = 18;

bool IsSpecialFile(string const & file) { return file == "." || file == ".."; }

bool CheckedGetFileType(string const & path, Platform::EFileType & type)
{
  Platform::EError const ret = Platform::GetFileType(path, type);
  if (ret != Platform::ERR_OK)
  {
    LOG(LERROR, ("Can't determine file type for", path, ":", ret));
    return false;
  }
  return true;
}

bool CheckedMkDir(string const & directory)
{
  Platform & platform = GetPlatform();
  Platform::EError const ret = platform.MkDir(directory);
  switch (ret)
  {
    case Platform::ERR_OK:
      return true;
    case Platform::ERR_FILE_ALREADY_EXISTS:
    {
      Platform::EFileType type;
      if (!CheckedGetFileType(directory, type))
        return false;
      if (type != Platform::FILE_TYPE_DIRECTORY)
      {
        LOG(LERROR, (directory, "exists, but not a directory:", type));
        return false;
      }
      return true;
    }
    default:
      LOG(LERROR, (directory, "can't be created:", ret));
      return false;
  }
}
}  // namespace

void CleanupMapsDirectory()
{
  Platform & platform = GetPlatform();

  string const mapsDir = platform.WritableDir();

  // Remove partially downloaded maps.
  {
    Platform::FilesList files;
    // .(downloading|resume|ready)[0-9]?$
    string const regexp = "\\.(downloading|resume|ready)[0-9]?$";
    platform.GetFilesByRegExp(mapsDir, regexp, files);
    for (string const & file : files)
      my::DeleteFileX(my::JoinFoldersToPath(mapsDir, file));
  }

  // Find and remove Brazil and Japan maps.
  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);
  for (LocalCountryFile & localFile : localFiles)
  {
    CountryFile const countryFile = localFile.GetCountryFile();
    if (countryFile.GetNameWithoutExt() == "Japan" || countryFile.GetNameWithoutExt() == "Brazil")
    {
      localFile.SyncWithDisk();
      localFile.DeleteFromDisk(TMapOptions::EMapWithCarRouting);
    }
  }

  // Try to delete empty folders.
  Platform::FilesList subdirs;
  Platform::GetFilesByType(mapsDir, Platform::FILE_TYPE_DIRECTORY, subdirs);
  for (string const & subdir : subdirs)
  {
    int64_t version;
    if (ParseVersion(subdir, version))
    {
      vector<string> files;
      string const subdirPath = my::JoinFoldersToPath(mapsDir, subdir);
      platform.GetFilesByType(subdirPath,
                              Platform::FILE_TYPE_REGULAR | Platform::FILE_TYPE_DIRECTORY, files);
      if (all_of(files.begin(), files.end(), &IsSpecialFile))
      {
        Platform::EError const ret = Platform::RmDir(subdirPath);
        ASSERT_EQUAL(Platform::ERR_OK, ret,
               ("Can't remove empty directory:", subdirPath, "error:", ret));
        UNUSED_VALUE(ret);
      }
    }
  }
}

void FindAllLocalMapsInDirectory(string const & directory, int64_t version,
                                 vector<LocalCountryFile> & localFiles)
{
  vector<string> files;
  Platform & platform = GetPlatform();

  platform.GetFilesByRegExp(directory, ".*\\" DATA_FILE_EXTENSION "$", files);
  for (string const & file : files)
  {
    // Remove DATA_FILE_EXTENSION and use base name as a country file name.
    string name = file;
    my::GetNameWithoutExt(name);
    localFiles.emplace_back(directory, CountryFile(name), version);
  }
}

void FindAllLocalMaps(vector<LocalCountryFile> & localFiles)
{
  localFiles.clear();

  Platform & platform = GetPlatform();

  string const directory = platform.WritableDir();
  FindAllLocalMapsInDirectory(directory, 0 /* version */, localFiles);

  Platform::FilesList subdirs;
  Platform::GetFilesByType(directory, Platform::FILE_TYPE_DIRECTORY, subdirs);
  for (string const & subdir : subdirs)
  {
    int64_t version;
    if (ParseVersion(subdir, version))
      FindAllLocalMapsInDirectory(my::JoinFoldersToPath(directory, subdir), version, localFiles);
  }

  // World and WorldCoasts can be stored in app bundle or in resources
  // directory, thus it's better to get them via Platform.
  for (string const & file : {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME})
  {
    bool found = false;
    for (LocalCountryFile const & localFile : localFiles)
    {
      if (localFile.GetCountryFile().GetNameWithoutExt() == file)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      try
      {
        ModelReaderPtr reader = platform.GetReader(file + DATA_FILE_EXTENSION);
        localFiles.emplace_back(my::GetDirectory(reader.GetName()), CountryFile(file),
                                0 /* version */);
      }
      catch (FileAbsentException const & e)
      {
        LOG(LERROR, ("Can't find map file for", file, "."));
      }
    }
  }
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

shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(CountryFile const & countryFile,
                                                         int64_t version)
{
  Platform & platform = GetPlatform();
  if (version == 0)
    return make_shared<LocalCountryFile>(platform.WritableDir(), countryFile, version);
  string const directory =
      my::JoinFoldersToPath(platform.WritableDir(), strings::to_string(version));
  if (!CheckedMkDir(directory))
    return shared_ptr<LocalCountryFile>();
  return make_shared<LocalCountryFile>(directory, countryFile, version);
}

// static
bool CountryIndexes::PreparePlaceOnDisk(LocalCountryFile const & localFile)
{
  return CheckedMkDir(IndexesDir(localFile));
}

// static
bool CountryIndexes::DeleteFromDisk(LocalCountryFile const & localFile)
{
  string const directory = IndexesDir(localFile);

  vector<string> files;
  Platform::GetFilesByRegExp(directory, "\\.*", files);
  for (string const & file : files)
  {
    if (IsSpecialFile(file))
      continue;
    string const path = my::JoinFoldersToPath(directory, file);
    if (!my::DeleteFileX(path))
      LOG(LERROR, ("Can't remove country index:", path));
  }

  Platform::EError const ret = Platform::RmDir(directory);
  if (ret != Platform::ERR_OK && ret != Platform::ERR_FILE_DOES_NOT_EXIST)
  {
    LOG(LERROR, ("Can't remove indexes directory:", directory, ret));
    return false;
  }
  return true;
}

// static
string CountryIndexes::GetPath(LocalCountryFile const & localFile, Index index)
{
  string const directory = IndexesDir(localFile);
  string const name = localFile.GetCountryFile().GetNameWithoutExt();
  char const * ext = nullptr;
  switch (index)
  {
    case Index::Bits:
      ext = ".bftsegbits";
      break;
    case Index::Nodes:
      ext = ".bftsegnodes";
      break;
    case Index::Offsets:
      ext = ".offsets";
      break;
  }
  return my::JoinFoldersToPath(directory, name + ext);
}

// static
string CountryIndexes::IndexesDir(LocalCountryFile const & localFile)
{
  return my::JoinFoldersToPath(localFile.GetDirectory(),
                               localFile.GetCountryFile().GetNameWithoutExt());
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
}
}  // namespace platform
