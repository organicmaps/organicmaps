#include "platform.hpp"
#include "settings.hpp"

#include "../coding/internal/file_data.hpp"
#include "../coding/file_reader.hpp"

#include "../base/logging.hpp"

#if !defined(OMIM_OS_WINDOWS_NATIVE) && !defined(OMIM_OS_BADA)
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "../base/start_mem_debug.hpp"


string ReadPathForFile(string const & writableDir,
                       string const & resourcesDir,
                       string const & file)
{
  string fullPath = writableDir + file;
  if (!GetPlatform().IsFileExists(fullPath))
  {
    fullPath = resourcesDir + file;
    if (!GetPlatform().IsFileExists(fullPath))
      MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
  }
  return fullPath;
}

ModelReader * BasePlatformImpl::GetReader(string const & file) const
{
    return new FileReader(ReadPathForFile(m_writableDir, m_resourcesDir, file), 10, 12);
}

bool BasePlatformImpl::GetFileSize(string const & file, uint64_t & size) const
{
  return my::GetFileSize(file, size);
}

void BasePlatformImpl::GetFilesInDir(string const & directory, string const & mask, FilesList & res) const
{
#if !defined(OMIM_OS_WINDOWS_NATIVE) && !defined(OMIM_OS_BADA)
  DIR * dir;
  struct dirent * entry;

  if ((dir = opendir(directory.c_str())) == NULL)
    return;

  // TODO: take wildcards into account...
  string mask_fixed = mask;
  if (mask_fixed.size() && mask_fixed[0] == '*')
    mask_fixed.erase(0, 1);

  do
  {
    if ((entry = readdir(dir)) != NULL)
    {
      string fname(entry->d_name);
      size_t index = fname.rfind(mask_fixed);
      if (index != string::npos && index == fname.size() - mask_fixed.size())
      {
        // TODO: By some strange reason under simulator stat returns -1,
        // may be because of symbolic links?..
        //struct stat fileStatus;
        //if (stat(string(directory + fname).c_str(), &fileStatus) == 0 &&
        //    (fileStatus.st_mode & S_IFDIR) == 0)
        //{
          res.push_back(fname);
        //}
      }
    }
  } while (entry != NULL);

  closedir(dir);
#else
  MYTHROW(NotImplementedException, ("Function not implemented"));
#endif
}

bool BasePlatformImpl::RenameFileX(string const & fOld, string const & fNew) const
{
  return my::RenameFileX(fOld, fNew);
}

void BasePlatformImpl::GetFontNames(FilesList & res) const
{
  res.clear();
  GetFilesInDir(m_resourcesDir, "*.ttf", res);

  sort(res.begin(), res.end());
}

double BasePlatformImpl::VisualScale() const
{
  return 1.0;
}

string BasePlatformImpl::SkinName() const
{
  return "basic.skn";
}

bool BasePlatformImpl::IsBenchmarking() const
{
  bool res = false;
  (void)Settings::Get("IsBenchmarking", res);

/*#ifndef OMIM_PRODUCTION
  if (res)
  {
    static bool first = true;
    if (first)
    {
      LOG(LCRITICAL, ("Benchmarking only defined in production configuration!"));
      first = false;
    }
    res = false;
  }
#endif*/

  return res;
}

bool BasePlatformImpl::IsVisualLog() const
{
  return false;
}

int BasePlatformImpl::ScaleEtalonSize() const
{
  return 512 + 256;
}

int BasePlatformImpl::MaxTilesCount() const
{
  return 80;
}

int BasePlatformImpl::TileSize() const
{
  return 256;
}

bool BasePlatformImpl::IsMultiThreadedRendering() const
{
  return true;
}
