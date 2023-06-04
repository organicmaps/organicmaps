/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "simple_logger.hpp"
#ifdef _MSC_VER
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

namespace
{
static const char COL_RESET[]{"\x1b[0m"};
static const char RED[]{"\x1b[31m"};
#ifndef NDEBUG
static const char YELLOW[]{"\x1b[33m"};
#endif
// static const char GREEN[] { "\x1b[32m"};
// static const char BLUE[] { "\x1b[34m"};
// static const char MAGENTA[] { "\x1b[35m"};
// static const char CYAN[] { "\x1b[36m"};
}

void LogPolicy::Unmute() { m_is_mute = false; }

void LogPolicy::Mute() { m_is_mute = true; }

bool LogPolicy::IsMute() const { return m_is_mute; }

LogPolicy &LogPolicy::GetInstance()
{
    static LogPolicy runningInstance;
    return runningInstance;
}

SimpleLogger::SimpleLogger() : level(logINFO) {}

std::mutex &SimpleLogger::get_mutex()
{
    static std::mutex mtx;
    return mtx;
}

std::ostringstream &SimpleLogger::Write(LogLevel lvl) noexcept
{
    std::lock_guard<std::mutex> lock(get_mutex());
    level = lvl;
    os << "[";
    switch (level)
    {
    case logWARNING:
        os << "warn";
        break;
    case logDEBUG:
#ifndef NDEBUG
        os << "debug";
#endif
        break;
    default: // logINFO:
        os << "info";
        break;
    }
    os << "] ";
    return os;
}

SimpleLogger::~SimpleLogger()
{
    std::lock_guard<std::mutex> lock(get_mutex());
    if (!LogPolicy::GetInstance().IsMute())
    {
        const bool is_terminal = static_cast<bool>(isatty(fileno(stdout)));
        switch (level)
        {
        case logWARNING:
            std::cerr << (is_terminal ? RED : "") << os.str() << (is_terminal ? COL_RESET : "")
                      << std::endl;
            break;
        case logDEBUG:
#ifndef NDEBUG
            std::cout << (is_terminal ? YELLOW : "") << os.str() << (is_terminal ? COL_RESET : "")
                      << std::endl;
#endif
            break;
        case logINFO:
        default:
            std::cout << os.str() << (is_terminal ? COL_RESET : "") << std::endl;
            break;
        }
    }
}
