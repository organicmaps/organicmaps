/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <boost/assert.hpp>

#include <cstdio>

#ifdef _MSC_VER
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#include <atomic>
#include <ostream>
#include <iostream>
#include <mutex>
#include <sstream>

enum LogLevel
{ logINFO,
  logWARNING,
  logDEBUG };

const char COL_RESET[] = "\x1b[0m";
const char RED[] = "\x1b[31m";
const char GREEN[] = "\x1b[32m";
const char YELLOW[] = "\x1b[33m";
const char BLUE[] = "\x1b[34m";
const char MAGENTA[] = "\x1b[35m";
const char CYAN[] = "\x1b[36m";

class LogPolicy
{
  public:
    void Unmute() { m_is_mute = false; }

    void Mute() { m_is_mute = true; }

    bool IsMute() const { return m_is_mute; }

    static LogPolicy &GetInstance()
    {
        static LogPolicy runningInstance;
        return runningInstance;
    }

    LogPolicy(const LogPolicy &) = delete;

  private:
    LogPolicy() : m_is_mute(true) {}
    std::atomic<bool> m_is_mute;
};

class SimpleLogger
{
  public:
    SimpleLogger() : level(logINFO) {}

    std::mutex& get_mutex()
    {
        static std::mutex m;
        return m;
    }


    std::ostringstream &Write(LogLevel l = logINFO)
    {
        std::lock_guard<std::mutex> lock(get_mutex());
        try
        {
            level = l;
            os << "[";
            switch (level)
            {
            case logINFO:
                os << "info";
                break;
            case logWARNING:
                os << "warn";
                break;
            case logDEBUG:
#ifndef NDEBUG
                os << "debug";
#endif
                break;
            default:
                BOOST_ASSERT_MSG(false, "should not happen");
                break;
            }
            os << "] ";
        }
        catch (...) {}
        return os;
    }

    virtual ~SimpleLogger()
    {
        std::lock_guard<std::mutex> lock(get_mutex());
        if (!LogPolicy::GetInstance().IsMute())
        {
            const bool is_terminal = ( 0 != isatty(fileno(stdout)) ? true : false);
            switch (level)
            {
            case logINFO:
                std::cout << os.str() << (is_terminal ? COL_RESET : "") << std::endl;
                break;
            case logWARNING:
                std::cerr << (is_terminal ? RED : "") << os.str() << (is_terminal ? COL_RESET : "")
                          << std::endl;
                break;
            case logDEBUG:
#ifndef NDEBUG
                std::cout << (is_terminal ? YELLOW : "") << os.str()
                          << (is_terminal ? COL_RESET : "") << std::endl;
#endif
                break;
            default:
                BOOST_ASSERT_MSG(false, "should not happen");
                break;
            }
        }
    }

  private:
    LogLevel level;
    std::ostringstream os;
};

#endif /* SIMPLE_LOGGER_H */
