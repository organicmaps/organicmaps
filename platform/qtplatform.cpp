#include "platform.hpp"

#include "../base/assert.hpp"
#include "../base/macros.hpp"
#include "../base/timer.hpp"
#include "../base/logging.hpp"
#include "../defines.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>

#include "../std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS)
  #include "../std/windows.hpp"
  #include <shlobj.h>
  #define DIR_SLASH "\\"

#elif defined(OMIM_OS_MAC)
  #include <stdlib.h>
  #include <mach-o/dyld.h>
  #include <sys/param.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/sysctl.h>
  #include <glob.h>
  #include <NSSystemDirectories.h>
  #define DIR_SLASH "/"

#elif defined(OMIM_OS_LINUX)
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #define DIR_SLASH "/"

#endif

#include "../base/start_mem_debug.hpp"

// default writable directory name for dev/standalone installs
#define MAPDATA_DIR "data"
// default writable dir name in LocalAppData or ~/Library/AppData/ folders
#define LOCALAPPDATA_DIR "MapsWithMe"
// default Resources read-only dir
#define RESOURCES_DIR "Resources"

/// Contains all resources needed to launch application
static char const * sRequiredResourcesList[] =
{
  "classificator.txt",
  "drawing_rules.bin",
  "basic.skn",
  "basic_highres.skn",
  "visibility.txt",
  "symbols_24.png",
  "symbols_48.png"//,
  //"countries.txt"
};

#ifdef OMIM_OS_MAC
string ExpandTildePath(char const * path)
{
//  ASSERT(path, ());
  glob_t globbuf;
  string result = path;

  if (::glob(path, GLOB_TILDE, NULL, &globbuf) == 0) //success
  {
    if (globbuf.gl_pathc > 0)
      result = globbuf.gl_pathv[0];

    globfree(&globbuf);
  }
  return result;
}
#endif

static bool GetUserWritableDir(string & outDir)
{
#if defined(OMIM_OS_WINDOWS)
  char pathBuf[MAX_PATH] = {0};
  if (SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pathBuf)))
  {
    outDir = pathBuf;
    ::CreateDirectoryA(outDir.c_str(), NULL);
    outDir += "\\" LOCALAPPDATA_DIR "\\";
    ::CreateDirectoryA(outDir.c_str(), NULL);
    return true;
  }

#elif defined(OMIM_OS_MAC)
  char pathBuf[PATH_MAX];
  NSSearchPathEnumerationState state = ::NSStartSearchPathEnumeration(NSApplicationSupportDirectory, NSUserDomainMask);
  while ((state = NSGetNextSearchPathEnumeration(state, pathBuf)))
  {
    outDir = ExpandTildePath(pathBuf);
    ::mkdir(outDir.c_str(), 0755);
    outDir += "/" LOCALAPPDATA_DIR "/";
    ::mkdir(outDir.c_str(), 0755);
    return true;
  }

#elif defined(OMIM_OS_LINUX)
  char * path = ::getenv("HOME");
  if (path)
  {
    outDir = path;
    outDir += "." LOCALAPPDATA_DIR "/";
    ::mkdir(outDir.c_str(), 0755);
    return true;
  }

#else
  #error "Implement code for your OS"

#endif
  return false;
}

/// @return full path including binary itself
static bool GetPathToBinary(string & outPath)
{
#if defined(OMIM_OS_WINDOWS)
    // get path to executable
    char pathBuf[MAX_PATH] = {0};
    if (0 < ::GetModuleFileNameA(NULL, pathBuf, MAX_PATH))
    {
      outPath = pathBuf;
      return true;
    }

#elif defined (OMIM_OS_MAC)
    char path[MAXPATHLEN] = {0};
    uint32_t pathSize = ARRAY_SIZE(path);
    if (0 == ::_NSGetExecutablePath(path, &pathSize))
    {
      char fixedPath[MAXPATHLEN] = {0};
      if (::realpath(path, fixedPath))
      {
        outPath = fixedPath;
        return true;
      }
    }

#elif defined (OMIM_OS_LINUX)
    char path[4096] = {0};
    if (0 < ::readlink("/proc/self/exe", path, ARRAY_SIZE(path)))
    {
      outPath = path;
      return true;
    }

#else
  #error "Add implementation for your operating system, please"

#endif
    return false;
}

static bool IsDirectoryWritable(string const & dir)
{
  if (!dir.empty())
  {
    QString qDir = dir.c_str();
    if (dir[dir.size() - 1] != '/' && dir[dir.size() - 1] != '\\')
      qDir.append('/');

    QTemporaryFile file(qDir + "XXXXXX");
    if (file.open())
      return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////
class QtPlatform : public Platform
{
  my::Timer m_startTime;
  string m_writableDir;
  string m_resourcesDir;

  /// Scans all upper directories for the presence of given directory
  /// @param[in] startPath full path to lowest file in hierarchy (usually binary)
  /// @param[in] dirName directory name we want to be present
  /// @return if not empty, contains full path to existing directory
  string DirFinder(string const & startPath, string const & dirName)
  {
    size_t slashPos = startPath.size();
    while ((slashPos = startPath.rfind(DIR_SLASH, slashPos - 1)) != string::npos)
    {
      string const dir = startPath.substr(0, slashPos) + DIR_SLASH + dirName + DIR_SLASH;
      if (IsFileExists(dir))
        return dir;
      if (slashPos == 0)
        break;
    }
    return string();
  }

  /// @TODO: add better validity check
  bool AreResourcesValidInDir(string const & dir)
  {
    for (size_t i = 0; i < ARRAY_SIZE(sRequiredResourcesList); ++i)
    {
      if (!IsFileExists(dir + sRequiredResourcesList[i]))
        return false;
    }
    return true;
  }

  bool GetOSSpecificResourcesDir(string const & exePath, string & dir)
  {
    dir = DirFinder(exePath, RESOURCES_DIR);
    return !dir.empty();
  }

  void InitResourcesDir(string & dir)
  {
    // Resources dir can be any "data" folder found in the nearest upper directory,
    // where all necessary resources files are present and accessible
    string exePath;
    CHECK( GetPathToBinary(exePath), ("Can't get full path to executable") );
    dir = DirFinder(exePath, MAPDATA_DIR);
    if (!dir.empty())
    {
      // check if all necessary resources are present in found dir
      if (AreResourcesValidInDir(dir))
        return;
    }
    // retrieve OS-specific resources dir
    CHECK( GetOSSpecificResourcesDir(exePath, dir), ("Can't retrieve resources directory") );
    CHECK( AreResourcesValidInDir(dir), ("Required resources are missing, terminating...") );
  }

  void InitWritableDir(string & dir)
  {
    // Writable dir can be any "data" folder found in the nearest upper directory
    // ./data     - For Windows portable builds
    // ../data
    // ../../data - For developer builds
    // etc. (for Mac in can be up to 6 levels above due to packages structure
    // and if no _writable_ "data" folder was found, User/Application Data/MapsWithMe will be used

    string path;
    CHECK( GetPathToBinary(path), ("Can't get full path to executable") );
    dir = DirFinder(path, MAPDATA_DIR);
    if (!(!dir.empty() && IsDirectoryWritable(dir)))
    {
      CHECK( GetUserWritableDir(dir), ("Can't get User's Application Data writable directory") );
    }
  }

public:
  QtPlatform()
  {
    InitWritableDir(m_writableDir);
    InitResourcesDir(m_resourcesDir);
  }

  virtual double TimeInSec() const
  {
    return m_startTime.ElapsedSeconds();
  }

  /// @return path to /data/ with ending slash
  virtual string WritableDir() const
  {
    return m_writableDir;
  }

  virtual string ReadPathForFile(char const * file) const
  {
    string fullPath = m_writableDir + file;
    if (!IsFileExists(fullPath))
    {
      fullPath = m_resourcesDir + file;
      if (!IsFileExists(fullPath))
        MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
    }
    return fullPath;
  }

  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const
  {
    outFiles.clear();
    QDir dir(directory.c_str(), mask.c_str(), QDir::Unsorted,
             QDir::Files | QDir::Readable | QDir::Dirs | QDir::NoDotAndDotDot);
    int const count = dir.count();
    for (int i = 0; i < count; ++i)
      outFiles.push_back(dir[i].toUtf8().data());
    return count;
  }

  virtual bool GetFileSize(string const & file, uint64_t & size) const
  {
    QFileInfo fileInfo(file.c_str());
    if (fileInfo.exists())
    {
      size = fileInfo.size();
      return true;
    }
    return false;
  }

  virtual bool RenameFileX(string const & original, string const & newName) const
  {
    return QFile::rename(original.c_str(), newName.c_str());
  }

  virtual int CpuCores() const
  {
#if defined(OMIM_OS_WINDOWS)
    SYSTEM_INFO sysinfo = {0};
    GetSystemInfo(&sysinfo);
    DWORD numCPU = sysinfo.dwNumberOfProcessors;
    if (numCPU >= 1)
      return static_cast<int>(numCPU);

#elif defined(OMIM_OS_MAC)
    int mib[2], numCPU = 0;
    size_t len = sizeof(numCPU);
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;
    sysctl(mib, 2, &numCPU, &len, NULL, 0);
    if (numCPU >= 1)
      return numCPU;
    // second try
    mib[1] = HW_NCPU;
    len = sizeof(numCPU);
    sysctl(mib, 2, &numCPU, &len, NULL, 0);
    if (numCPU >= 1)
     return numCPU;

#elif defined(OMIM_OS_LINUX)
    long numCPU = sysconf(_SC_NPROCESSORS_ONLN);
    if (numCPU >= 1)
      return static_cast<int>(numCPU);
#endif
    return 1;
  }

  double VisualScale() const
  {
    return 1.0;
  }

  string const SkinName() const
  {
    return "basic.skn";
  }

  bool IsMultiSampled() const
  {
    return true;
  }

  bool DoPeriodicalUpdate() const
  {
    return true;
  }

  double PeriodicalUpdateInterval() const
  {
    return 0.3;
  }

  vector<string> GetFontNames() const
  {
     vector<string> res;

     string fontFolder = m_resourcesDir;
     //string fontFolder = "/Library/Fonts/";

     GetFilesInDir(fontFolder, "*.ttf", res);

     sort(res.begin(), res.end());

     for (size_t i = 0; i < res.size(); ++i)
       res[i] = fontFolder + res[i];

     return res;
  }

  bool IsBenchmarking() const
  {
    bool res = false;
#ifndef OMIM_PRODUCTION
    if (res)
    {
      static bool first = true;
      if (first)
      {
        LOG(LCRITICAL, ("benchmarking only defined in production configuration"));
        first = false;
      }
      res = false;
    }
#endif
    return res;
  }

  string const DeviceID() const
  {
    return "DesktopVersion";
  }

  bool IsVisualLog() const
  {
    return false;
  }

  unsigned ScaleEtalonSize() const
  {
    return 512 + 256;
  }
};

extern "C" Platform & GetPlatform()
{
  static QtPlatform platform;
  return platform;
}
