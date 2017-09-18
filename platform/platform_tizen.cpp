#include "platform.hpp"
#include "constants.hpp"
#include "platform_unix_impl.hpp"
#include "tizen_utils.hpp"
#include "http_thread_tizen.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "coding/file_reader.hpp"

#include "base/logging.hpp"

#include <FAppApp.h>
#include "tizen/inc/FIo.hpp"


Platform::Platform()
{
  Tizen::App::App * pApp = Tizen::App::App::GetInstance();
  // init directories
  string app_root = FromTizenString(pApp->GetAppRootPath());

  m_writableDir = FromTizenString(pApp->GetAppDataPath());
  m_resourcesDir = FromTizenString(pApp->GetAppResourcePath());
  m_settingsDir = m_writableDir + "settings/";
  Tizen::Io::Directory::Create(m_settingsDir.c_str(), true);

  m_tmpDir = m_writableDir + "tmp/";
  Tizen::Io::Directory::Create(m_tmpDir.c_str(), true);

  LOG(LDEBUG, ("App directory:", app_root));
  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
  LOG(LDEBUG, ("Client ID:", UniqueClientId()));

  m_flags[HAS_BOOKMARKS] = true;
  m_flags[HAS_ROTATION] = true;
  m_flags[HAS_ROUTING] = true;
}

// static
void Platform::MkDir(string const & dirName) const
{
  Tizen::Io::Directory::Create(dirName.c_str(), true);
}

string Platform::UniqueClientId() const
{
  Tizen::App::App * pApp = Tizen::App::App::GetInstance();
  return FromTizenString(pApp->GetAppId());
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  /// @todo
  fn();
}

void Platform::RunAsync(TFunctor const & fn, Priority p)
{
  /// @todo
  fn();
}

ModelReader * Platform::GetReader(string const & file, string const & searchScope) const
{
  return new FileReader(ReadPathForFile(file, searchScope),
                        READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  pl::EnumerateFilesByRegExp(directory, regexp, res);
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

int Platform::VideoMemoryLimit() const
{
  return 10 * 1024 * 1024;
}

int Platform::PreCachingDepth() const
{
  return 3;
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  // @TODO Add implementation
  return EConnectionType::CONNECTION_NONE;
}

Platform::ChargingStatus Platform::GetChargingStatus()
{
  return Platform::ChargingStatus::Unknown;
}

void Platform::GetSystemFontNames(FilesList & res) const
{
}

extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}

class HttpThread;

namespace downloader
{

class IHttpThreadCallback;

HttpThread * CreateNativeHttpThread(string const & url,
                                    downloader::IHttpThreadCallback & cb,
                                    int64_t beg,
                                    int64_t end,
                                    int64_t size,
                                    string const & pb)
{
  HttpThread * pRes = new HttpThread(url, cb, beg, end, size, pb);
  return pRes;
}

void DeleteNativeHttpThread(HttpThread * request)
{
  delete request;
}

}
