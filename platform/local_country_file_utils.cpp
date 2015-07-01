#include "platform/local_country_file_utils.hpp"

#include "platform/platform.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "std/algorithm.hpp"
#include "std/cctype.hpp"

namespace platform
{
namespace
{
size_t const kMaxTimestampLength = 18;

bool IsSpecialFile(string const & file) { return file == "." || file == ".."; }
}  // namespace

void CleanupMapsDirectory()
{
  Platform & platform = GetPlatform();

  string const mapsDir = platform.WritableDir();

  // Remove partially downloaded maps.
  {
    Platform::FilesList files;
    string const regexp = "\\" DATA_FILE_EXTENSION "\\.(downloading2?$|resume2?$)";
    platform.GetFilesByRegExp(mapsDir, regexp, files);
    for (string const & file : files)
      FileWriter::DeleteFileX(file);
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
      localFile.DeleteFromDisk();
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
  vector<LocalCountryFile> allFiles;
  Platform & platform = GetPlatform();
  vector<string> baseDirectories = {
      platform.ResourcesDir(), platform.WritableDir(),
  };
  sort(baseDirectories.begin(), baseDirectories.end());
  baseDirectories.erase(unique(baseDirectories.begin(), baseDirectories.end()),
                        baseDirectories.end());
  for (string const & directory : baseDirectories)
  {
    FindAllLocalMapsInDirectory(directory, 0 /* version */, allFiles);

    Platform::FilesList subdirs;
    Platform::GetFilesByType(directory, Platform::FILE_TYPE_DIRECTORY, subdirs);
    for (string const & subdir : subdirs)
    {
      int64_t version;
      if (ParseVersion(subdir, version))
        FindAllLocalMapsInDirectory(my::JoinFoldersToPath(directory, subdir), version, allFiles);
    }
  }
#if defined(OMIM_OS_ANDROID)
  // On Android World and WorldCoasts can be stored in alternative /Android/obb/ path.
  for (string const & file : {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME})
  {
    bool found = false;
    for (LocalCountryFile const & localFile : allFiles)
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
        allFiles.emplace_back(my::GetDirectory(reader.GetName()), CountryFile(file),
                              0 /* version */);
      }
      catch (FileAbsentException const & e)
      {
        LOG(LERROR, ("Can't find map file for", file, "."));
      }
    }
  }
#endif  // defined(OMIM_OS_ANDROID)

  localFiles.insert(localFiles.end(), allFiles.begin(), allFiles.end());
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
  switch (platform.MkDir(directory))
  {
    case Platform::ERR_OK:
      return make_shared<LocalCountryFile>(directory, countryFile, version);
    case Platform::ERR_FILE_ALREADY_EXISTS:
    {
      Platform::EFileType type;
      if (Platform::GetFileType(directory, type) != Platform::ERR_OK ||
          type != Platform::FILE_TYPE_DIRECTORY)
      {
        return shared_ptr<LocalCountryFile>();
      }
      return make_shared<LocalCountryFile>(directory, countryFile, version);
    }
    default:
      return shared_ptr<LocalCountryFile>();
  };
}
}  // namespace platform
