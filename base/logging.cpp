#include "assert.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "timer.hpp"

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
    static char const * names[] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };
    static size_t const len[] =   {    5,      4,        7,        5,         8     };
    ostringstream out;
    out << "LOG ";
    out << names[level];

    //int64_t const milliseconds = static_cast<int64_t>(s_Timer.ElapsedSeconds() * 1000 + 0.5);
    //std::cerr << " " << std::setw(6) << milliseconds / 1000 << "." << std::setw(4) << std::setiosflags(std::ios::left) << (milliseconds % 1000) << std::resetiosflags(std::ios::left);

    double const sec = s_Timer.ElapsedSeconds();
    out << " " << std::setfill(' ') << std::setw(16 - len[level]) << sec;

    out << " " << DebugPrint(srcPoint) << msg << endl;

    string const outString = out.str();
    std::cerr << outString;
#ifdef OMIM_OS_WINDOWS
    OutputDebugStringA(outString.c_str());
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
