#pragma once

#include "../base/logging.hpp"

// If you uncomment the line bellow the application log will be written to a file.
// You'll fild the file in MapsWithMe directory on Android platform and in Documents on iOS.
// #define MWM_LOG_TO_FILE

void LogMessageFile(my::LogLevel level, my::SrcPoint const & srcPoint, string const & msg);
void LogMemoryInfo();
