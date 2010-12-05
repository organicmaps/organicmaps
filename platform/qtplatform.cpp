#include "platform.hpp"

#include "../base/assert.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "../std/target_os.hpp"
#include "../std/timer.hpp"

#if defined(OMIM_OS_WINDOWS)
  #include "../std/windows.hpp"

#elif defined(OMIM_OS_MAC)
  #include <stdlib.h>
  #include <mach-o/dyld.h>
  #include <sys/param.h>
  #include <sys/types.h>
  #include <sys/sysctl.h>

#elif defined(OMIM_OS_LINUX)
  #include <unistd.h>
#endif

#include "../base/start_mem_debug.hpp"

class QtPlatform : public Platform
{
  boost::timer m_startTime;
  string m_workingDir;

public:
  virtual double TimeInSec()
  {
    return m_startTime.elapsed();
  }

  /// @return path to /data/ with ending slash
  virtual string WorkingDir()
  {
    if (m_workingDir.empty())
    {
      size_t slashesToFind = 3;

#ifdef OMIM_OS_WINDOWS
      {
        char path[MAX_PATH] = {0};
        ::GetModuleFileNameA(NULL, path, MAX_PATH);
        m_workingDir = path;
        // replace backslashes with slashes
        std::replace(m_workingDir.begin(), m_workingDir.end(), '\\', '/');
      }

#elif defined(OMIM_OS_MAC)
      {
        char path[MAXPATHLEN] = {0};
        uint32_t pathSize = sizeof(path);
        _NSGetExecutablePath(path, &pathSize);
        char fixedPath[MAXPATHLEN] = {0};
        realpath(path, fixedPath);
        m_workingDir = fixedPath;

        // If we are in application bundle, remove the path inside application bundle.
        // First remove the application name
        m_workingDir.erase(m_workingDir.rfind("/", string::npos));
        slashesToFind -= 1;
        // Then - /Contents/MacOS
        string const bundlePath = "/Contents/MacOS";
        if (m_workingDir.size() > bundlePath.size() &&
            m_workingDir.substr(m_workingDir.size() - bundlePath.size()) == bundlePath)
        {
          m_workingDir.erase(m_workingDir.size() - bundlePath.size());
          slashesToFind += 1;
        }
      }

#elif defined(OMIM_OS_LINUX)
      char path[256] = {0};
      readlink("/proc/self/exe", path, sizeof(path)/sizeof(path[0]));
      m_workingDir = path;
#endif

      size_t slashIndex = string::npos;
      for (size_t i = 0; i < slashesToFind; ++i)
        slashIndex = m_workingDir.rfind('/', slashIndex) - 1;
      ASSERT ( slashIndex != string::npos, (m_workingDir) );

      m_workingDir.erase(slashIndex + 2);
      m_workingDir.append("data/");
    }

    ASSERT ( !m_workingDir.empty(), ("Some real shit happens") );
    return m_workingDir;
  }

  virtual string ResourcesDir()
  {
    return WorkingDir();
  }

  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles)
  {
    QDir dir(directory.c_str(), mask.c_str(), QDir::Unsorted,
             QDir::Files | QDir::Readable | QDir::Dirs | QDir::NoDotAndDotDot);
    int const count = dir.count();
    for (int i = 0; i < count; ++i)
      outFiles.push_back(dir[i].toUtf8().data());
    return count;
  }

  virtual bool GetFileSize(string const & file, uint64_t & size)
  {
    QFileInfo fileInfo(file.c_str());
    if (fileInfo.exists())
    {
      size = fileInfo.size();
      return true;
    }
    return false;
  }

  virtual bool RenameFileX(string const & original, string const & newName)
  {
    return QFile::rename(original.c_str(), newName.c_str());
  }

  virtual int CpuCores()
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
};

extern "C" Platform & GetPlatform()
{
  static QtPlatform platform;
  return platform;
}
