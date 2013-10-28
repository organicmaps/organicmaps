#include "platform.hpp"
#include "platform_unix_impl.hpp"
#include "constants.hpp"

#include "../coding/zip_reader.hpp"
#include "../coding/file_name_utils.hpp"

#include "../base/logging.hpp"
#include "../base/thread.hpp"
#include "../base/regexp.hpp"
#include "../base/string_utils.hpp"

#include <unistd.h>     // for sysconf


Platform::Platform()
{
  /// @see initialization routine in android/jni/com/.../Platform.hpp
}

namespace
{

enum SourceT { EXTERNAL_RESOURCE, RESOURCE, WRITABLE_PATH, FULL_PATH };

size_t GetSearchSources(string const & file, SourceT (&arr)[4])
{
  size_t ret = 0;
  string const ext = my::GetFileExtension(file);
  ASSERT ( !ext.empty(), () );

  if (ext == DATA_FILE_EXTENSION)
  {
    bool const isWorld =
        strings::StartsWith(file, WORLD_COASTS_FILE_NAME) ||
        strings::StartsWith(file, WORLD_FILE_NAME);

    if (isWorld)
      arr[ret++] = EXTERNAL_RESOURCE;
    arr[ret++] = WRITABLE_PATH;
    if (isWorld)
      arr[ret++] = RESOURCE;
  }
  else if (ext == FONT_FILE_EXTENSION)
  {
    // system fonts have absolute unix path
    if (strings::StartsWith(file, "/"))
      arr[ret++] = FULL_PATH;
    else
    {
      arr[ret++] = EXTERNAL_RESOURCE;
      arr[ret++] = WRITABLE_PATH;
      arr[ret++] = RESOURCE;
    }
  }
  else
    arr[ret++] = RESOURCE;

  return ret;
}

#ifdef DEBUG
class DbgLogger
{
  string const & m_file;
  SourceT m_src;
public:
  DbgLogger(string const & file) : m_file(file) {}
  void SetSource(SourceT src) { m_src = src; }
  ~DbgLogger()
  {
    LOG(LDEBUG, ("Source for file", m_file, "is", m_src));
  }
};
#endif

}

ModelReader * Platform::GetReader(string const & file, string const & searchScope) const
{
  // @TODO now we handle only two specific cases needed to release guides ads
  if (searchScope == "w")
  {
    string const path = m_writableDir + file;
    if (IsFileExistsByFullPath(path))
      return new FileReader(path, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
    MYTHROW(FileAbsentException, ("File not found", file));
  }
  else if (searchScope == "r")
  {
    return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
  // @TODO refactor code below and some other parts too, like fonts and maps detection and loading,
  // as it can be done much better


  SourceT sources[4];
  size_t const n = GetSearchSources(file, sources);

#ifdef DEBUG
  DbgLogger logger(file);
#endif

  for (size_t i = 0; i < n; ++i)
  {
#ifdef DEBUG
    logger.SetSource(sources[i]);
#endif

    switch (sources[i])
    {
    case EXTERNAL_RESOURCE:
    {
      for (size_t j = 0; j < m_extResFiles.size(); ++j)
      {
        try
        {
          return new ZipFileReader(m_extResFiles[j], file,
                                   READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
        }
        catch (Reader::OpenException const &)
        {
        }
      }
      break;
    }

    case WRITABLE_PATH:
    {
      string const path = m_writableDir + file;
      if (IsFileExistsByFullPath(path))
        return new FileReader(path, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
      break;
    }

    case FULL_PATH:
      if (IsFileExistsByFullPath(file))
        return new FileReader(file);
      break;

    default:
      ASSERT_EQUAL ( sources[i], RESOURCE, () );
      ASSERT_EQUAL ( file.find("assets/"), string::npos, () );
      return new ZipFileReader(m_resourcesDir, "assets/" + file);
    }
  }

  MYTHROW(FileAbsentException, ("File not found", file));
  return 0;
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  {
    // Get files list inside zip file
    typedef ZipFileReader::FileListT FilesT;
    FilesT fList;
    ZipFileReader::FilesList(directory, fList);

    regexp::RegExpT exp;
    regexp::Create(regexp, exp);

    for (FilesT::iterator it = fList.begin(); it != fList.end(); ++it)
    {
      string & name = it->first;
      if (regexp::IsExist(name, exp))
      {
        // Remove assets/ prefix - clean files are needed for fonts white/blacklisting logic
        size_t const ASSETS_LENGTH = 7;
        if (name.find("assets/") == 0)
          name.erase(0, ASSETS_LENGTH);

        res.push_back(name);
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
