#include "platform.hpp"
#include "platform_unix_impl.hpp"

#include "../base/logging.hpp"

#include <dirent.h>
#include <sys/stat.h>

#ifdef OMIM_OS_ANDROID
  #include <sys/vfs.h>
#else
  #include <sys/mount.h>
#endif


bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  struct stat s;
  return stat(filePath.c_str(), &s) == 0;
}

bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  struct stat s;
  if (stat(filePath.c_str(), &s) == 0)
  {
    size = s.st_size;
    return true;
  }
  else return false;
}

Platform::TStorageStatus Platform::GetWritableStorageStatus(uint64_t neededSize)
{
  struct statfs st;
  int const ret = statfs(m_writableDir.c_str(), &st);

  LOG(LDEBUG, ("statfs return = ", ret,
               "; block size = ", st.f_bsize,
               "; blocks available = ", st.f_bavail));

  if (ret != 0)
    return STORAGE_DISCONNECTED;

  /// @todo May be add additional storage space.
  if (st.f_bsize * st.f_bavail < neededSize)
    return NOT_ENOUGH_SPACE;

  return STORAGE_OK;
}

namespace pl
{

string GetFixedMask(string const & mask)
{
  // Filter out according to the mask.
  // @TODO we don't support wildcards at the moment
  if (!mask.empty() && mask[0] == '*')
    return string(mask.c_str() + 1);
  else
    return mask;
}

void EnumerateFilesInDir(string const & directory, string const & mask, vector<string> & res)
{
  DIR * dir;
  struct dirent * entry;
  if ((dir = opendir(directory.c_str())) == NULL)
    return;

  string const fixedMask = GetFixedMask(mask);

  while ((entry = readdir(dir)) != 0)
  {
    string const fname(entry->d_name);
    size_t const index = fname.rfind(fixedMask);
    if ((index != string::npos) && (index == fname.size() - fixedMask.size()))
      res.push_back(fname);
  }

  closedir(dir);
}

}
