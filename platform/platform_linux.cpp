#include "platform/platform.hpp"

#include "platform/socket.hpp"

#include "coding/file_reader.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <string>

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

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
bool IsWelcomeExist(string const & directory)
{
  return Platform::IsFileExistsByFullPath(base::JoinPath(directory, "welcome.html"));
}

bool GetHomeDir(string & outPath)
{
  char const * homePath = ::getenv("HOME");
  if (homePath == nullptr)
    return false;
  outPath = homePath;
  return true;
}

}  // namespace

namespace platform
{
unique_ptr<Socket> CreateSocket()
{
  return unique_ptr<Socket>();
}
} // namespace platform


Platform::Platform()
{
  // Init directories.
  string execPath, homePath;
  CHECK(GetBinaryDir(execPath), ("Can't retrieve path to executable"));
  CHECK(GetHomeDir(homePath), ("Can't retrieve home path"));

  m_settingsDir = base::JoinPath(homePath, ".config", "OMaps");

  if (!IsFileExistsByFullPath(base::JoinPath(m_settingsDir, SETTINGS_FILE_NAME)))
  {
    if (!MkDirRecursively(m_settingsDir))
      MYTHROW(FileSystemException, ("Can't create directory", m_settingsDir));
  }

  m_settingsDir += '/';

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
    m_writableDir = base::JoinPath(homePath, ".local", "share", "OMaps");
    if (!MkDirRecursively(m_writableDir))
      MYTHROW(FileSystemException, ("Can't create directory:", m_writableDir));
  }
  else
  {
    string dirs[] = {
      "./data",                                     // check symlink in the current folder
      "../data",                                    // check if we are in the 'build' folder inside repo
      base::JoinPath(execPath, "..", "..", "data"), // check symlink from bundle?
      base::JoinPath(execPath, "..", "share"),      // installed version with packages
      base::JoinPath(execPath, "..", "OMaps")       // installed version without packages
    };

    for (auto const & dir : dirs)
    {
      if (IsWelcomeExist(dir))
      {
        m_resourcesDir = dir;
        m_writableDir = (writableDir != nullptr) ? writableDir : m_resourcesDir;
        break;
      }
    }
  }

  ValidateWritableAndResourceDirs();

  char const * tmpDir = ::getenv("TMPDIR");
  if (tmpDir)
  {
    m_tmpDir = tmpDir;
  }
  else
  {
    /// @todo Don't have other good ideas here ..
    m_tmpDir = base::JoinPath(homePath, "tmp");
    if (!MkDirRecursively(m_tmpDir))
      MYTHROW(FileSystemException, ("Can't create tmp directory:", m_tmpDir));
  }
  m_tmpDir += '/';

  m_guiThread = make_unique<platform::GuiThread>();

  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

string Platform::DeviceModel() const
{
  return {};
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  int socketFd = socket(AF_INET, SOCK_STREAM, 0);
  SCOPE_GUARD(closeSocket, bind(&close, socketFd));
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

uint8_t Platform::GetBatteryLevel()
{
  // This value is always 100 for desktop.
  return 100;
}

void Platform::GetSystemFontNames(FilesList & res) const
{
  char const * fontsWhitelist[] = {
    "Roboto-Medium.ttf",
    "Roboto-Regular.ttf",
    "DroidSansFallback.ttf",
    "DroidSansFallbackFull.ttf",
    "DroidSans.ttf",
    "DroidSansArabic.ttf",
    "DroidSansSemc.ttf",
    "DroidSansSemcCJK.ttf",
    "DroidNaskh-Regular.ttf",
    "Lohit-Bengali.ttf",
    "Lohit-Devanagari.ttf",
    "Lohit-Tamil.ttf",
    "PakType Naqsh.ttf",
    "wqy-microhei.ttc",
    "Jomolhari.ttf",
    "Padauk.ttf",
    "KhmerOS.ttf",
    "Umpush.ttf",
    "DroidSansThai.ttf",
    "DroidSansArmenian.ttf",
    "DroidSansEthiopic-Regular.ttf",
    "DroidSansGeorgian.ttf",
    "DroidSansHebrew-Regular.ttf",
    "DroidSansHebrew.ttf",
    "DroidSansJapanese.ttf",
    "LTe50872.ttf",
    "LTe50259.ttf",
    "DevanagariOTS.ttf",
    "FreeSans.ttf",
    "DejaVuSans.ttf",
    "arial.ttf",
    "AbyssinicaSIL-R.ttf",
  };

  string systemFontsPath[] = {
    "/usr/share/fonts/truetype/roboto/",
    "/usr/share/fonts/truetype/droid/",
    "/usr/share/fonts/truetype/dejavu/",
    "/usr/share/fonts/truetype/ttf-dejavu/",
    "/usr/share/fonts/truetype/wqy/",
    "/usr/share/fonts/truetype/freefont/",
    "/usr/share/fonts/truetype/padauk/",
    "/usr/share/fonts/truetype/dzongkha/",
    "/usr/share/fonts/truetype/ttf-khmeros-core/",
    "/usr/share/fonts/truetype/tlwg/",
    "/usr/share/fonts/truetype/abyssinica/",
    "/usr/share/fonts/truetype/paktype/",
  };

  for (auto font : fontsWhitelist)
  {
    for (auto sysPath : systemFontsPath)
    {
      string path = sysPath + font;
      if (IsFileExistsByFullPath(path))
      {
        LOG(LINFO, ("Found usable system font", path));
        res.push_back(std::move(path));
      }
    }
  }
}
