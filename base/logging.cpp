#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/mutex.hpp"
#include "base/thread.hpp"
#include "base/timer.hpp"

#include "std/iomanip.hpp"
#include "std/iostream.hpp"
#include "std/mutex.hpp"
#include "std/sstream.hpp"
#include "std/target_os.hpp"
#include "std/windows.hpp"


namespace my
{
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
      s << " " << setfill(' ') << setw(static_cast<int>(16 - m_lens[level])) << sec << " ";
    }
  };

  mutex g_logMutex;

  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
  {
    lock_guard<mutex> lock(g_logMutex);

    static LogHelper logger;

    ostringstream out;
    logger.WriteProlog(out, level);

    out << DebugPrint(srcPoint) << msg << endl;
    std::cerr << out.str();

    CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
  }

  void LogMessageTests(LogLevel level, SrcPoint const &, string const & msg)
  {
    lock_guard<mutex> lock(g_logMutex);

    ostringstream out;
    out << msg << endl;
    std::cerr << out.str();

    CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
  }

  LogMessageFn LogMessage = &LogMessageDefault;

  LogMessageFn SetLogMessageFn(LogMessageFn fn)
  {
    std::swap(LogMessage, fn);
    return fn;
  }

#ifdef DEBUG
  LogLevel g_LogLevel = LDEBUG;
  LogLevel g_LogAbortLevel = LERROR;
#else
  LogLevel g_LogLevel = LINFO;
  LogLevel g_LogAbortLevel = LCRITICAL;
#endif
}
