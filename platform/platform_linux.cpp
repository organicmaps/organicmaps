#include "platform.hpp"

#include "../base/logging.hpp"

#include <stdlib.h>
#include <unistd.h>
#include "../std/fstream.hpp"


#include <sys/stat.h>
#include <sys/types.h>


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

  string home;
  home = ::getenv("HOME");

  m_settingsDir = home + "/.config/MapsWithMe/";

  if (!IsFileExistsByFullPath(m_settingsDir + SETTINGS_FILE_NAME))
  {
    mkdir((home + "/.config/").c_str(), 0755);
    mkdir(m_settingsDir.c_str(), 0755);
  }

  m_writableDir = home + "/.local/share/MapsWithMe/";
  mkdir((home + "/.local/").c_str(), 0755);
  mkdir((home + "/.local/share/").c_str(), 0755);
  mkdir(m_writableDir.c_str(), 0755);

  char * resDir = ::getenv("MWM_RESOURCES_DIR");
  if (resDir)
    m_resourcesDir = resDir;
  else
  {
    // installed version
    if (IsFileExistsByFullPath("/usr/share/MapsWithMe/eula.html"))
      m_resourcesDir = "/usr/share/MapsWithMe";
    // developer builds with symlink
    if (IsFileExistsByFullPath(path + "../../data/eula.html")){
      m_resourcesDir = path + "../../data";
      m_writableDir = m_resourcesDir;
    }
    // developer builds without symlink
    if (IsFileExistsByFullPath(path + "../../../omim/data/eula.html"))
    {
      m_resourcesDir = path + "../../../omim/data";
      m_writableDir = m_resourcesDir;
    }
    // portable installations
    else if (IsFileExistsByFullPath(path + "/eula.html"))
    {
      m_resourcesDir = path;
      m_writableDir = m_resourcesDir;
      m_settingsDir = m_resourcesDir;
    }
  }
  m_resourcesDir += '/';
  m_settingsDir += '/';

  char * tmpDir = ::getenv("TMPDIR");
  if (tmpDir)
    m_tmpDir = tmpDir;
  else
    m_tmpDir = "/tmp";
  m_tmpDir += '/';

  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
  LOG(LDEBUG,  ("Client ID:", UniqueClientId()));
}

int Platform::CpuCores() const
{
  const long numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  if (numCPU >= 1)
    return static_cast<int>(numCPU);
  return 1;
}

string Platform::UniqueClientId() const
{  
  string machinefile = "/var/lib/dbus/machine-id";
  if (IsFileExistsByFullPath("/etc/machine-id"))
    machinefile = "/etc/machine-id";

  std::ifstream ifs(machinefile.c_str());
  string content( (std::istreambuf_iterator<char>(ifs) ),
                  (std::istreambuf_iterator<char>()    ) );

  return content.substr(0,32);
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
