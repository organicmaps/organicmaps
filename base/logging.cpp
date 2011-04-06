#include "assert.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "timer.hpp"
#include "../std/iostream.hpp"
#include "../std/iomanip.hpp"

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

#ifdef OMIM_OS_BADA
  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
  {
    /// @todo need bada console
    LogCheckIfErrorLevel(level);
  }
#else
  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
  {
    // TODO: Make LogMessageDefault() thread-safe?
    static Timer s_Timer;
    char const * names[] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };
    std::cerr << "LOG ";
    if (level >= 0 && level <= static_cast<int>(ARRAY_SIZE(names)))
      std::cerr << names[level];
    else
      std::cerr << level;

    //int64_t const milliseconds = static_cast<int64_t>(s_Timer.ElapsedSeconds() * 1000 + 0.5);
    //std::cerr << " " << std::setw(6) << milliseconds / 1000 << "." << std::setw(4) << std::setiosflags(std::ios::left) << (milliseconds % 1000) << std::resetiosflags(std::ios::left);

    double const sec = s_Timer.ElapsedSeconds();
    std::cerr << " " << std::setfill(' ') << std::setw(10) << sec;

    std::cerr << " " << srcPoint.FileName() << ":" << srcPoint.Line() << " " << srcPoint.Function() << "() " << msg << endl;

    LogCheckIfErrorLevel(level);
  }
#endif

  LogMessageFn LogMessage = &LogMessageDefault;

  void SetLogMessageFn(LogMessageFn fn)
  {
    LogMessage = fn;
  };

#ifdef DEBUG
  LogLevel g_LogLevel = LDEBUG;
#else
  LogLevel g_LogLevel = LINFO;
#endif
}
