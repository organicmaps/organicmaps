#include "platform/local_country_file_utils.hpp"

#include "platform/platform.hpp"
#include "coding/file_name_utils.hpp"

#include "std/algorithm.hpp"
#include "std/cctype.hpp"

namespace local_country_file_utils
{
namespace
{
size_t const kMaxTimestampLength = 18;
}  // namespace

void FindAllLocalMapsInDirectory(string const & directory, int64_t version,
                                 vector<LocalCountryFile> & localFiles)
{
  vector<string> files;
  Platform & platform = GetPlatform();

  platform.GetFilesByRegExp(directory, ".*\\" DATA_FILE_EXTENSION "$", files);
  for (string const & file : files)
  {
    string name = file;
    my::GetNameWithoutExt(name);

    CountryFile const countryFile(name);
    localFiles.emplace_back(directory, countryFile, version);
  }
}

void FindAllLocalMaps(vector<LocalCountryFile> & localFiles)
{
  vector<LocalCountryFile> allFiles;
  Platform & platform = GetPlatform();
  for (string const & directory : {platform.ResourcesDir(), platform.WritableDir()})
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
#ifdef OMIM_OS_ANDROID
  // On Android World and WorldCoasts can be stored in alternative /Android/obb/ path.
  FindAllLocalMapsInDirectory("/Android/obb/", 0 /* version */, allFiles);
#endif
  sort(allFiles.begin(), allFiles.end());
  allFiles.erase(unique(allFiles.begin(), allFiles.end()), allFiles.end());
  localFiles.insert(localFiles.end(), allFiles.begin(), allFiles.end());
}

bool ParseVersion(string const & s, int64_t & version)
{
  if (s.empty() || s.size() > kMaxTimestampLength)
    return false;
  if (!all_of(s.begin(), s.end(), [](char c) -> bool
                                  {
        return isdigit(c);
      }))
    return false;
  version = 0;
  for (char c : s)
    version = version * 10 + c - '0';
  return true;
}
}  // namespace local_country_file_utils
