
#include "platform.hpp"

#include <FAppApp.h>
#include <FBaseUtilStringUtil.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
  #include <FIo.h>
#pragma clang diagnostic pop

#include "constants.hpp"
#include "platform_unix_impl.hpp"

#include "../base/logging.hpp"
#include "../coding/file_reader.hpp"

#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

string FromTizenString(Tizen::Base::String const & str_tizen)
{
  string utf8Str;
    if (str_tizen.GetLength() == 0)
      return utf8Str;
//    str_tizen.GetPointer();
    Tizen::Base::ByteBuffer* pBuffer = Tizen::Base::Utility::StringUtil::StringToUtf8N(str_tizen);
    if (pBuffer != null)
    {
      int byteCount = pBuffer->GetLimit();
      char* chPtrBuf = new char[byteCount + 1];
      if (chPtrBuf != null) {
        pBuffer->GetArray((byte*) chPtrBuf, 0, byteCount);
        utf8Str.assign(chPtrBuf, byteCount - 1);
        delete[] chPtrBuf;
      }
      if (pBuffer != null)
        delete pBuffer;
    }
    return utf8Str;
}


/// @return directory where binary resides, including slash at the end
static bool GetBinaryFolder(string & outPath)
{
  Tizen::App::App * pApp = Tizen::App::App::GetInstance();
  outPath = FromTizenString(pApp->GetAppRootPath());
  return true;
}

Platform::Platform()
{
  // init directories
  string app_root;
  CHECK(GetBinaryFolder(app_root), ("Can't retrieve path to executable"));


  string home = app_root;
  home += "data/";
  m_settingsDir = home + ".config/";

  Tizen::Io::Directory::Create(m_settingsDir.c_str(), true);

  m_writableDir = home + ".local/share/";
  Tizen::Io::Directory::Create((home + ".local/").c_str(), true);
  Tizen::Io::Directory::Create(m_writableDir.c_str(), true);

  m_resourcesDir = app_root + "res/";
  m_writableDir = home;

  m_tmpDir = home + "tmp/";
  Tizen::Io::Directory::Create(m_tmpDir.c_str(), true);

  LOG(LDEBUG, ("App directory:", app_root));
  LOG(LDEBUG, ("Home directory:", home));
  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
  LOG(LDEBUG, ("Client ID:", UniqueClientId()));
}

int Platform::CpuCores() const
{
  /// @todo
//  const long numCPU = sysconf(_SC_NPROCESSORS_ONLN);
//  if (numCPU >= 1)
//    return static_cast<int>(numCPU);
  return 1;
}

string Platform::UniqueClientId() const
{
  /// @todo

//  string machineFile = "/var/lib/dbus/machine-id";
//  if (IsFileExistsByFullPath("/etc/machine-id"))
//    machineFile = "/etc/machine-id";

//  if (IsFileExistsByFullPath(machineFile))
//  {
//    string content;
//    FileReader(machineFile).ReadAsString(content);
//    return content.substr(0, 32);
//  }
//  else
    return "n0dbus0n0lsb00000000000000000000";

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
    return GetFileSizeByFullPath(ReadPathForFile(fileName, "wr"), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

////////////////////////////////////////////////////////////////////////
extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
