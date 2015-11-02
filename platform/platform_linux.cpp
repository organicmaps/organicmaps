#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/bind.hpp"

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

namespace
{
// Web service ip to check internet connection. Now it's a mail.ru ip.
char constexpr kSomeWorkingWebServer[] = "217.69.139.202";

// Returns directory where binary resides, including slash at the end.
bool GetBinaryDir(string & outPath)
{
  char path[4096] = {};
  if (::readlink("/proc/self/exe", path, ARRAY_SIZE(path)) <= 0)
    return false;
  outPath = path;
  outPath.erase(outPath.find_last_of('/') + 1);
  return true;
}

// Returns true if EULA file exists in directory.
bool IsEulaExist(string const & directory)
{
  return Platform::IsFileExistsByFullPath(my::JoinFoldersToPath(directory, "eula.html"));
}
}  // namespace

Platform::Platform()
{
  // Init directories.
  string path;
  CHECK(GetBinaryDir(path), ("Can't retrieve path to executable"));

  char const * homePath = ::getenv("HOME");
  string const home(homePath ? homePath : "");

  m_settingsDir = my::JoinFoldersToPath({home, ".config"}, "MapsWithMe");

  if (!IsFileExistsByFullPath(my::JoinFoldersToPath(m_settingsDir, SETTINGS_FILE_NAME)))
  {
    MkDir(my::JoinFoldersToPath(home, ".config"));
    MkDir(m_settingsDir.c_str());
  }

  m_writableDir = my::JoinFoldersToPath({home, ".local", "share"}, "MapsWithMe");
  MkDir(my::JoinFoldersToPath(home, ".local"));
  MkDir(my::JoinFoldersToPath({home, ".local"}, "share"));
  MkDir(m_writableDir);

  char const * resDir = ::getenv("MWM_RESOURCES_DIR");
  if (resDir)
  {
    m_resourcesDir = resDir;
  }
  else
  {
    string const devBuildWithSymlink = my::JoinFoldersToPath({path, "..", ".."}, "data");
    string const devBuildWithoutSymlink =
        my::JoinFoldersToPath({path, "..", "..", "..", "omim"}, "data");
    string const installedVersionWithPackages = my::JoinFoldersToPath({path, ".."}, "share");
    string const installedVersionWithoutPackages =
        my::JoinFoldersToPath({path, ".."}, "MapsWithMe");
    string const customInstall = path;

    if (IsEulaExist(devBuildWithSymlink))
    {
      m_resourcesDir = devBuildWithSymlink;
      m_writableDir = m_resourcesDir;
    }
    else if (IsEulaExist(devBuildWithoutSymlink))
    {
      m_resourcesDir = devBuildWithoutSymlink;
      m_writableDir = m_resourcesDir;
    }
    else if (IsEulaExist(installedVersionWithPackages))
    {
      m_resourcesDir = installedVersionWithPackages;
    }
    else if (IsEulaExist(installedVersionWithoutPackages))
    {
      m_resourcesDir = installedVersionWithoutPackages;
    }
    else if (IsEulaExist(customInstall))
    {
      m_resourcesDir = path;
    }
  }
  m_resourcesDir += '/';
  m_settingsDir += '/';
  m_writableDir += '/';

  char const * tmpDir = ::getenv("TMPDIR");
  if (tmpDir)
    m_tmpDir = tmpDir;
  else
    m_tmpDir = "/tmp";
  m_tmpDir += '/';

  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
  LOG(LDEBUG, ("Client ID:", UniqueClientId()));
}

string Platform::UniqueClientId() const
{
  string machineFile = "/var/lib/dbus/machine-id";
  if (IsFileExistsByFullPath("/etc/machine-id"))
    machineFile = "/etc/machine-id";

  if (IsFileExistsByFullPath(machineFile))
  {
    string content;
    FileReader(machineFile).ReadAsString(content);
    return content.substr(0, 32);
  }

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

Platform::EConnectionType Platform::ConnectionStatus()
{
  int socketFd = socket(AF_INET, SOCK_STREAM, 0);
  MY_SCOPE_GUARD(closeSocket, bind(&close, socketFd));
  if (socketFd < 0)
    return EConnectionType::CONNECTION_NONE;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, kSomeWorkingWebServer, &addr.sin_addr);

  if (connect(socketFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    return EConnectionType::CONNECTION_NONE;

  return EConnectionType::CONNECTION_WIFI;
}
