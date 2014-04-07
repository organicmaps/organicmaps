#include "assert.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "timer.hpp"
#include "thread.hpp"
#include "mutex.hpp"

#include "../std/iostream.hpp"
#include "../std/iomanip.hpp"
#include "../std/sstream.hpp"
#include "../std/target_os.hpp"
#include "../std/windows.hpp"

namespace my
{
  void LogCheckIfErrorLevel(LogLevel level)
  {
#ifdef DEBUG
    if (level >= LERROR)
#else
    if (level >= LCRITICAL)
#endif
    {
      CHECK(false, ("Error level is too serious", level));
    }
  }

#ifdef OMIM_OS_TIZEN
#include <FBaseLog.h>
  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
  {
    /// @todo need bada console

    ostringstream out;
    out << DebugPrint(srcPoint) << msg << endl;
    switch (level)
    {
    case LDEBUG:
      AppLogDebug(out.str().c_str());
      break;
    case LINFO:
    case LWARNING:
      AppLog(out.str().c_str());
      break;
    case LERROR:
    case LCRITICAL:
      AppLogException(out.str().c_str());
    }

  }
#else

  class LogHelper
  {
    int m_threadsCount;
    map<threads::ThreadID, int> m_threadID;

    int GetThreadID()
    {
      int & id = m_threadID[threads::GetCurrentThreadID()];
      if (id == 0)
        id = ++m_threadsCount;
      return id;
    }

    my::Timer m_timer;

    char const * m_names[5];
    size_t m_lens[5];

  public:
    threads::Mutex m_mutex;

    LogHelper() : m_threadsCount(0)
    {
      m_names[0] = "DEBUG"; m_lens[0] = 5;
      m_names[1] = "INFO"; m_lens[1] = 4;
      m_names[2] = "WARNING"; m_lens[2] = 7;
      m_names[3] = "ERROR"; m_lens[3] = 5;
      m_names[4] = "CRITICAL"; m_lens[4] = 8;
    }

    void WriteProlog(ostream & s, LogLevel level)
    {
      s << "LOG";

      s << " TID(" << GetThreadID() << ")";
      s << " " << m_names[level];

      double const sec = m_timer.ElapsedSeconds();
      s << " " << setfill(' ') << setw(16 - m_lens[level]) << sec << " ";
    }
  };

  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
  {
    static LogHelper logger;

    threads::MutexGuard guard(logger.m_mutex);
    UNUSED_VALUE(guard);

    ostringstream out;
    logger.WriteProlog(out, level);

    out << DebugPrint(srcPoint) << msg << endl;

    std::cerr << out.str();

#ifdef OMIM_OS_WINDOWS
    OutputDebugStringA(out.str().c_str());
#endif
    LogCheckIfErrorLevel(level);
  }
#endif

  LogMessageFn LogMessage = &LogMessageDefault;

  LogMessageFn SetLogMessageFn(LogMessageFn fn)
  {
    std::swap(LogMessage, fn);
    return fn;
  }

#ifdef DEBUG
  LogLevel g_LogLevel = LDEBUG;
#else
  LogLevel g_LogLevel = LINFO;
#endif
}
