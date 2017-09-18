#include "platform/platform.hpp"
#include "platform/socket.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_name_utils.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include "std/bind.hpp"
#include "std/string.hpp"

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

string HomeDir()
{
  char const * homePath = ::getenv("HOME");
  return homePath != nullptr ? string(homePath) : "";
}

// Returns the default path to the writable dir, creating the dir if needed.
// An exception is thrown if the default dir is not already there and we were unable to create it.
string DefaultWritableDir()
{
  auto const home = HomeDir();
  string const result = my::JoinFoldersToPath({home, ".local", "share"}, "MapsWithMe");
  if (!Platform::MkDirChecked(my::JoinFoldersToPath(home, ".local")) ||
      !Platform::MkDirChecked(my::JoinFoldersToPath({home, ".local"}, "share")) ||
      !Platform::MkDirChecked(result))
  {
    MYTHROW(FileSystemException, ("Can't create directory", result));
  }

  return result;
}
}  // namespace

namespace platform
{
unique_ptr<Socket> CreateSocket()
{
  return unique_ptr<Socket>();
}
}

Platform::Platform()
{
  // Init directories.
  string path;
  CHECK(GetBinaryDir(path), ("Can't retrieve path to executable"));

  m_settingsDir = my::JoinFoldersToPath({HomeDir(), ".config"}, "MapsWithMe");

  if (!IsFileExistsByFullPath(my::JoinFoldersToPath(m_settingsDir, SETTINGS_FILE_NAME)))
  {
    auto const configDir = my::JoinFoldersToPath(HomeDir(), ".config");
    if (!MkDirChecked(configDir))
      MYTHROW(FileSystemException, ("Can't create directory", configDir));
    if (!MkDirChecked(m_settingsDir))
      MYTHROW(FileSystemException, ("Can't create directory", m_settingsDir));
  }

  char const * resDir = ::getenv("MWM_RESOURCES_DIR");
  char const * writableDir = ::getenv("MWM_WRITABLE_DIR");
  if (resDir && writableDir)
  {
    m_resourcesDir = resDir;
    m_writableDir = writableDir;
  }
  else if (resDir)
  {
    m_resourcesDir = resDir;
    m_writableDir = DefaultWritableDir();
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
      m_writableDir = writableDir != nullptr ? writableDir : m_resourcesDir;
    }
    else if (IsEulaExist(devBuildWithoutSymlink))
    {
      m_resourcesDir = devBuildWithoutSymlink;
      m_writableDir = writableDir != nullptr ? writableDir : m_resourcesDir;
    }
    else if (IsEulaExist(installedVersionWithPackages))
    {
      m_resourcesDir = installedVersionWithPackages;
      m_writableDir = writableDir != nullptr ? writableDir : DefaultWritableDir();
    }
    else if (IsEulaExist(installedVersionWithoutPackages))
    {
      m_resourcesDir = installedVersionWithoutPackages;
      m_writableDir = writableDir != nullptr ? writableDir : DefaultWritableDir();
    }
    else if (IsEulaExist(customInstall))
    {
      m_resourcesDir = path;
      m_writableDir = writableDir != nullptr ? writableDir : DefaultWritableDir();
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

  m_guiThread = make_unique<platform::GuiThread>();

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

Platform::ChargingStatus Platform::GetChargingStatus()
{
  return Platform::ChargingStatus::Plugged;
}

void Platform::SetGuiThread(unique_ptr<base::TaskLoop> guiThread)
{
  m_guiThread = move(guiThread);
}
