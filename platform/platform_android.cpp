#include "platform.hpp"

#include "../coding/zip_reader.hpp"

#include "../base/logging.hpp"
#include "../base/thread.hpp"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>


Platform::Platform() : m_impl(0)
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
    return new BaseZipFileReaderType(ReadPathForFile(file));
  else
  {
    ASSERT_EQUAL(file.find("assets/"), string::npos, ("Do not use assets/, only file name"));
    return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
}

namespace
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
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  {
    // Get files list inside zip file
    FilesList fList;
    ZipFileReader::FilesList(directory, fList);

    string const fixedMask = GetFixedMask(mask);

    for (FilesList::iterator it = fList.begin(); it != fList.end(); ++it)
    {
      if (it->find(fixedMask) != string::npos)
      {
        // Remove assets/ prefix - clean files are needed for fonts white/blacklisting logic
        static size_t const ASSETS_LENGTH = 7;
        if (it->find("assets/") == 0)
          it->erase(0, ASSETS_LENGTH);

        res.push_back(*it);
      }
    }
  }
  else
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

int Platform::CpuCores() const
{
  static long const numCPU = sysconf(_SC_NPROCESSORS_CONF);

  // for debugging only. _SC_NPROCESSORS_ONLN could change, so
  // we should test whether _SC_NPROCESSORS_CONF could change too

  long const newNumCPU = sysconf(_SC_NPROCESSORS_CONF);

  if (newNumCPU != numCPU)
    LOG(LWARNING, ("initially retrived", numCPU, "and now got", newNumCPU, "processors"));

  return (numCPU > 1 ? static_cast<int>(numCPU) : 1);
}

string Platform::DeviceName() const
{
  return "Android";
}

void Platform::GetFontNames(FilesList & res) const
{
  string arr[] = { WritableDir(), ResourcesDir() };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    LOG(LDEBUG, ("Searching for fonts in", arr[i]));
    GetFilesInDir(arr[i], "*.ttf", res);
  }

  sort(res.begin(), res.end());
  res.erase(unique(res.begin(), res.end()), res.end());

  LOG(LDEBUG, ("Font files:", (res)));
}

int Platform::ScaleEtalonSize() const
{
  return 512 + 256;
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

bool Platform::IsFeatureSupported(string const & feature) const
{
  if (feature == "search")
  {
    /// @todo add additional checks for apk, protection, etc ...
    return true;
  }
  return false;
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  /// @todo
  fn();
}

namespace
{
  class SelfDeleteRoutine : public threads::IRoutine
  {
    typedef Platform::TFunctor FnT;
    FnT m_fn;

  public:
    SelfDeleteRoutine(FnT const & fn) : m_fn(fn) {}

    virtual void Do()
    {
      m_fn();
      delete this;
    }
  };
}

void Platform::RunAsync(TFunctor const & fn, Priority p)
{
  UNUSED_VALUE(p);

  // We don't need to store thread handler in POSIX. Just create and run.
  threads::Thread().Create(new SelfDeleteRoutine(fn));
}
