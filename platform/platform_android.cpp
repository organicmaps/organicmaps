#include "platform/constants.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_unix_impl.hpp"
#include "platform/settings.hpp"

#include "coding/zip_reader.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"
#include "base/string_utils.hpp"

#include "std/regex.hpp"

#include <unistd.h>     // for sysconf
#include <sys/stat.h>

Platform::Platform()
{
  /// @see initialization routine in android/jni/com/.../Platform.hpp
}

namespace
{
enum SourceT
{
  EXTERNAL_RESOURCE,
  RESOURCE,
  WRITABLE_PATH,
  SETTINGS_PATH,
  FULL_PATH,
  SOURCE_COUNT
};

bool IsResource(string const & file, string const & ext)
{
  if (ext == DATA_FILE_EXTENSION)
  {
    return (strings::StartsWith(file, WORLD_COASTS_FILE_NAME) ||
            strings::StartsWith(file, WORLD_COASTS_OBSOLETE_FILE_NAME) ||
            strings::StartsWith(file, WORLD_FILE_NAME));
  }
  else if (ext == BOOKMARKS_FILE_EXTENSION ||
           ext == ROUTING_FILE_EXTENSION ||
           file == SETTINGS_FILE_NAME)
  {
    return false;
  }

  return true;
}

size_t GetSearchSources(string const & file, string const & searchScope,
                        SourceT (&arr)[SOURCE_COUNT])
{
  size_t ret = 0;

  for (size_t i = 0; i < searchScope.size(); ++i)
  {
    switch (searchScope[i])
    {
    case 'w': arr[ret++] = WRITABLE_PATH; break;
    case 'r': arr[ret++] = RESOURCE; break;
    case 'e': arr[ret++] = EXTERNAL_RESOURCE; break;
    case 's': arr[ret++] = SETTINGS_PATH; break;
    case 'f':
      if (strings::StartsWith(file, "/"))
        arr[ret++] = FULL_PATH;
      break;
    default : CHECK(false, ("Unsupported searchScope:", searchScope)); break;
    }
  }

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

unique_ptr<ModelReader> Platform::GetReader(string const & file, string const & searchScope) const
{
  string const ext = my::GetFileExtension(file);
  ASSERT(!ext.empty(), ());

  uint32_t const logPageSize = (ext == DATA_FILE_EXTENSION) ? READER_CHUNK_LOG_SIZE : 10;
  uint32_t const logPageCount = (ext == DATA_FILE_EXTENSION) ? READER_CHUNK_LOG_COUNT : 4;

  SourceT sources[SOURCE_COUNT];
  size_t n = 0;

  if (searchScope.empty())
  {
    // Default behaviour - use predefined scope for resource files and writable path for all others.

    if (IsResource(file, ext))
      n = GetSearchSources(file, m_androidDefResScope, sources);
    else
    {
      // Add source for map files and other dynamic stored data.
      sources[n++] = WRITABLE_PATH;
      sources[n++] = FULL_PATH;
    }
  }
  else
  {
    // Use passed scope as client wishes.
    n = GetSearchSources(file, searchScope, sources);
  }

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
      for (size_t j = 0; j < m_extResFiles.size(); ++j)
      {
        try
        {
          return make_unique<ZipFileReader>(m_extResFiles[j], file, logPageSize, logPageCount);
        }
        catch (Reader::OpenException const &)
        {
        }
      }
      break;

    case WRITABLE_PATH:
    {
      string const path = m_writableDir + file;
      if (IsFileExistsByFullPath(path))
        return make_unique<FileReader>(path, logPageSize, logPageCount);
      break;
    }

    case SETTINGS_PATH:
    {
      string const path = m_settingsDir + file;
      if (IsFileExistsByFullPath(path))
        return make_unique<FileReader>(path, logPageSize, logPageCount);
      break;
    }

    case FULL_PATH:
      if (IsFileExistsByFullPath(file))
        return make_unique<FileReader>(file, logPageSize, logPageCount);
      break;

    case RESOURCE:
      ASSERT_EQUAL(file.find("assets/"), string::npos, ());
      try
      {
        return make_unique<ZipFileReader>(m_resourcesDir, "assets/" + file, logPageSize, logPageCount);
      }
      catch (Reader::OpenException const & e)
      {
        LOG(LWARNING, ("Can't get reader:", e.what()));
      }
      break;

    default:
      CHECK(false, ("Unsupported source:", sources[i]));
      break;
    }
  }

  LOG(LWARNING, ("Can't get reader for:", file));
  MYTHROW(FileAbsentException, ("File not found", file));
  return nullptr;
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

string Platform::DeviceModel() const
{
  return {};
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  if (ZipFileReader::IsZip(directory))
  {
    // Get files list inside zip file
    typedef ZipFileReader::FileListT FilesT;
    FilesT fList;
    ZipFileReader::FilesList(directory, fList);

    regex exp(regexp);

    for (FilesT::iterator it = fList.begin(); it != fList.end(); ++it)
    {
      string & name = it->first;
      if (regex_search(name.begin(), name.end(), exp))
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
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Can't get file size for:", fileName));
    return false;
  }
}

// static
Platform::EError Platform::MkDir(string const & dirName)
{
  if (0 != mkdir(dirName.c_str(), 0755))
    return ErrnoToError();
  return Platform::ERR_OK;
}

void Platform::SetupMeasurementSystem() const
{
  auto units = measurement_utils::Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units))
    return;
  // @TODO Add correct implementation
  units = measurement_utils::Units::Metric;
  settings::Set(settings::kMeasurementUnits, units);
}

/// @see implementation of methods below in android/jni/com/.../Platform.cpp
//  void RunOnGuiThread(base::TaskLoop::Task && task);
//  void RunOnGuiThread(base::TaskLoop::Task const & task);
