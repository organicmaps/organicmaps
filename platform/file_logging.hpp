#pragma once

#include "base/logging.hpp"

#include <string>

// @todo this functionality is not located in logging.hpp since file_logging uses FileWriter which depends on coding lib.
// At the same time loging is located in base and I do not want base depens on several more libs.
// Probably it's a good idea to move all logging functionality (logging, file_logging, mem_logging and so on)
// to a special subsystem which depends on base and coding.

// If you uncomment the line bellow the application log will be written to a file.
// You'll find a log file (logging_<date><time>.log) in the MapsWithMe directory on Android platform and in Documents on iOS.
// #define MWM_LOG_TO_FILE

// Writing information about free memory to log file.
// #ifdef DEBUG
// # define OMIM_ENABLE_LOG_MEMORY_INFO
// #endif

void LogMessageFile(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & msg);
void LogMemoryInfo();

#ifdef OMIM_ENABLE_LOG_MEMORY_INFO
# define LOG_MEMORY_INFO()  LogMemoryInfo()
#else
# define LOG_MEMORY_INFO()  do {} while(false)
#endif
