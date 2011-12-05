#include "platform.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/zip_reader.hpp"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

Platform::Platform()
{}

Platform::~Platform()
{}

/// @warning doesn't work for files inside .apk (zip)!!!
bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  struct stat s;
  return stat(filePath.c_str(), &s) == 0;
}

ModelReader * Platform::GetReader(string const & file) const
{
  if (IsFileExistsByFullPath(m_writableDir + file))
    return new FileReader(ReadPathForFile(file), 10, 12);
  else
  {
    ASSERT_EQUAL(file.find("assets/"), string::npos, ("Do not use assets/, only file name"));
    return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  { // Get files list inside zip file
    res = ZipFileReader::FilesList(directory);
    // filter out according to the mask
    // @TODO we don't support wildcards at the moment
    string fixedMask = mask;
    if (fixedMask.size() && fixedMask[0] == '*')
      fixedMask.erase(0, 1);
    for (FilesList::iterator it = res.begin(); it != res.end();)
    {
      if (it->find(fixedMask) == string::npos)
        it = res.erase(it);
      else
      {
        // Remove assets/ prefix - clean files are needed for fonts white/blacklisting logic
        static size_t const ASSETS_LENGTH = 7;
        if (it->find("assets/") == 0)
          it->erase(0, ASSETS_LENGTH);
        ++it;
      }
    }
  }
  else
  {
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
            res.push_back(fname);
        }
      }
    } while (entry != NULL);

    closedir(dir);
  }
}

int Platform::CpuCores() const
{
  long const numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  if (numCPU >= 1)
    return static_cast<int>(numCPU);
  return 1;

}

string Platform::DeviceName() const
{
  return "Android";
}

double Platform::VisualScale() const
{
  return 1.5;
}

string Platform::SkinName() const
{
  return "basic_mdpi.skn";
}

void Platform::GetFontNames(FilesList & res) const
{
  GetFilesInDir(ResourcesDir(), "*.ttf", res);
  sort(res.begin(), res.end());
}

int Platform::ScaleEtalonSize() const
{
  return 512 + 256;
}

int Platform::TileSize() const
{
  return 256;
}

int Platform::MaxTilesCount() const
{
  return 120;
}

int Platform::VideoMemoryLimit() const
{
  return 10 * 1024 * 1024;
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    size = ReaderPtr<Reader>(GetReader(fileName)).Size();
    return true;
  }
  catch (RootException const &)
  {
    return false;
  }
}

/// @warning doesn't work for files inside .apk (zip)!!!
bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  struct stat s;
  if (stat(filePath.c_str(), &s) == 0)
  {
    size = s.st_size;
    return true;
  }
  return false;
}

string Platform::UniqueClientId() const
{
  return "@TODO";
}
