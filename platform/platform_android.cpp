#include "platform.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/zip_reader.hpp"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

static string ReadPathForFile(string const & writableDir,
    string const & resourcesDir, string const & file)
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

Platform::Platform()
{}

Platform::~Platform()
{}

static bool IsFilePresent(string const & file)
{
  struct stat s;
  return stat(file.c_str(), &s) == 0;
}

ModelReader * Platform::GetReader(string const & file) const
{
  // can't use Platform::IsFileExists here to avoid recursion
  if (IsFilePresent(m_writableDir + file))
    return new FileReader(ReadPathForFile(m_writableDir, m_resourcesDir, file), 10, 12);
  else
  { // paths from GetFilesInDir will already contain "assets/"
    if (file.find("assets/") != string::npos)
      return new ZipFileReader(m_resourcesDir, file);
    else
      return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & res) const
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
        ++it;
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
  return 1.3;
}

string Platform::SkinName() const
{
  return "basic.skn";
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

bool Platform::GetFileSize(string const & file, uint64_t & size) const
{
  try
  {
    size = ReaderPtr<Reader>(GetReader(file)).Size();
    return true;
  }
  catch (RootException const &)
  {
    return false;
  }
}
