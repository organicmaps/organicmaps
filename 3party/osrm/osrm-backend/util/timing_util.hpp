/*

Copyright (c) 2014, Project OSRM contributors
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

#ifndef TIMING_UTIL_HPP
#define TIMING_UTIL_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

struct GlobalTimer
{
    GlobalTimer() : time(0) {}
    std::atomic<uint64_t> time;
};

class GlobalTimerFactory
{
  public:
    static GlobalTimerFactory &get()
    {
        static GlobalTimerFactory instance;
        return instance;
    }

    GlobalTimer &getGlobalTimer(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(map_mutex);
        return timer_map[name];
    }

  private:
    std::mutex map_mutex;
    std::unordered_map<std::string, GlobalTimer> timer_map;
};

#define GLOBAL_TIMER_AQUIRE(_X)                                                                    \
    auto &_X##_global_timer = GlobalTimerFactory::get().getGlobalTimer(#_X)
#define GLOBAL_TIMER_RESET(_X) _X##_global_timer.time = 0
#define GLOBAL_TIMER_START(_X) TIMER_START(_X)
#define GLOBAL_TIMER_STOP(_X)                                                                      \
    TIMER_STOP(_X);                                                                                \
    _X##_global_timer.time += TIMER_NSEC(_X)
#define GLOBAL_TIMER_NSEC(_X) static_cast<double>(_X##_global_timer.time)
#define GLOBAL_TIMER_USEC(_X) (_X##_global_timer.time / 1000.0)
#define GLOBAL_TIMER_MSEC(_X) (_X##_global_timer.time / 1000.0 / 1000.0)
#define GLOBAL_TIMER_SEC(_X) (_X##_global_timer.time / 1000.0 / 1000.0 / 1000.0)

#define TIMER_START(_X) auto _X##_start = std::chrono::steady_clock::now(), _X##_stop = _X##_start
#define TIMER_STOP(_X) _X##_stop = std::chrono::steady_clock::now()
#define TIMER_NSEC(_X)                                                                             \
    std::chrono::duration_cast<std::chrono::nanoseconds>(_X##_stop - _X##_start).count()
#define TIMER_USEC(_X)                                                                             \
    std::chrono::duration_cast<std::chrono::microseconds>(_X##_stop - _X##_start).count()
#define TIMER_MSEC(_X)                                                                             \
    (0.000001 *                                                                                    \
     std::chrono::duration_cast<std::chrono::nanoseconds>(_X##_stop - _X##_start).count())
#define TIMER_SEC(_X)                                                                              \
    (0.000001 *                                                                                    \
     std::chrono::duration_cast<std::chrono::microseconds>(_X##_stop - _X##_start).count())
#define TIMER_MIN(_X)                                                                              \
    std::chrono::duration_cast<std::chrono::minutes>(_X##_stop - _X##_start).count()

#endif // TIMING_UTIL_HPP
