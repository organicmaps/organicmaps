#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "coding/file_reader.hpp"

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
}  // namespace

/// @return directory where binary resides, including slash at the end
static bool GetBinaryFolder(string & outPath)
{
  char path[4096] = {0};
  if (0 < ::readlink("/proc/self/exe", path, ARRAY_SIZE(path)))
  {
    outPath = path;
    outPath.erase(outPath.find_last_of('/') + 1);
    return true;
  }
  return false;
}

Platform::Platform()
{
  // init directories
  string path;
  CHECK(GetBinaryFolder(path), ("Can't retrieve path to executable"));

  char const * homePath = ::getenv("HOME");
  string const home(homePath ? homePath : "");

  m_settingsDir = home + "/.config/MapsWithMe";

  if (!IsFileExistsByFullPath(m_settingsDir + SETTINGS_FILE_NAME))
  {
    mkdir((home + "/.config/").c_str(), 0755);
    mkdir(m_settingsDir.c_str(), 0755);
  }

  m_writableDir = home + "/.local/share/MapsWithMe";
  mkdir((home + "/.local/").c_str(), 0755);
  mkdir((home + "/.local/share/").c_str(), 0755);
  mkdir(m_writableDir.c_str(), 0755);

  char const * resDir = ::getenv("MWM_RESOURCES_DIR");
  if (resDir)
    m_resourcesDir = resDir;
  else
  {
    // developer builds with symlink
    if (IsFileExistsByFullPath(path + "../../data/eula.html"))
    {
      m_resourcesDir = path + "../../data";
      m_writableDir = m_resourcesDir;
    }
    // developer builds without symlink
    else if (IsFileExistsByFullPath(path + "../../../omim/data/eula.html"))
    {
      m_resourcesDir = path + "../../../omim/data";
      m_writableDir = m_resourcesDir;
    }
    // installed version - /opt/MapsWithMe and unpacked packages
    else if (IsFileExistsByFullPath(path + "../share/eula.html"))
      m_resourcesDir = path + "../share";
    // installed version
    else if (IsFileExistsByFullPath(path + "../share/MapsWithMe/eula.html"))
      m_resourcesDir = path + "../share/MapsWithMe";
    // all-nearby installs
    else if (IsFileExistsByFullPath(path + "/eula.html"))
      m_resourcesDir = path;
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
  else
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
