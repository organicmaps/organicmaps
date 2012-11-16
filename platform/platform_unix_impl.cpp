#include "platform.hpp"
#include "platform_unix_impl.hpp"

#include "../base/logging.hpp"
#include "../base/regexp.hpp"

#include <dirent.h>
#include <sys/stat.h>

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <sys/mount.h>
#else
  #include <sys/vfs.h>
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

Platform::TStorageStatus Platform::GetWritableStorageStatus(uint64_t neededSize) const
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

void EnumerateFilesByRegExp(string const & directory, string const & regexp,
                            vector<string> & res)
{
  DIR * dir;
  struct dirent * entry;
  if ((dir = opendir(directory.c_str())) == NULL)
    return;

  regexp::RegExpT exp;
  regexp::Create(regexp, exp);

  while ((entry = readdir(dir)) != 0)
  {
    string const name(entry->d_name);
    if (regexp::IsExist(name, exp))
      res.push_back(name);
  }

  closedir(dir);
}

}
