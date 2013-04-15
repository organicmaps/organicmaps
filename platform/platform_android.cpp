#include "platform.hpp"
#include "platform_unix_impl.hpp"
#include "constants.hpp"

#include "../coding/zip_reader.hpp"

#include "../base/logging.hpp"
#include "../base/thread.hpp"
#include "../base/regexp.hpp"

#include <unistd.h>


Platform::Platform()
{
  /// @see initialization routine in android/jni/com/.../Platform.hpp
}

ModelReader * Platform::GetReader(string const & file) const
{
  if (IsFileExistsByFullPath(m_writableDir + file))
  {
    return new FileReader(ReadPathForFile(file),
                          READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
  }
  if (IsFileExistsByFullPath(file))
  {
    return new FileReader(ReadPathForFile(file),
                          READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
  }
  else
  {
    ASSERT_EQUAL ( file.find("assets/"), string::npos, () );

    /// @note If you push some maps to the bundle, it's better to set
    /// better values for chunk size and chunks count. @see constants.hpp
    return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  {
    // Get files list inside zip file
    FilesList fList;
    ZipFileReader::FilesList(directory, fList);

    regexp::RegExpT exp;
    regexp::Create(regexp, exp);

    for (FilesList::iterator it = fList.begin(); it != fList.end(); ++it)
    {
      if (regexp::IsExist(*it, exp))
      {
        // Remove assets/ prefix - clean files are needed for fonts white/blacklisting logic
        size_t const ASSETS_LENGTH = 7;
        if (it->find("assets/") == 0)
          it->erase(0, ASSETS_LENGTH);

        res.push_back(*it);
      }
    }
  }
  else
    pl::EnumerateFilesByRegExp(directory, regexp, res);
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

int Platform::VideoMemoryLimit() const
{
  return 10 * 1024 * 1024;
}

int Platform::PreCachingDepth() const
{
  return 3;
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
