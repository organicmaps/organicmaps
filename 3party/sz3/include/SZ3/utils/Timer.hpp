//
// Created by Kai Zhao on 10/30/20.
//

#ifndef SZ3_TIMER_HPP
#define SZ3_TIMER_HPP

#include <chrono>
#include <iostream>
#include <string>

namespace SZ3 {
class Timer {
   public:
    Timer() = default;

    Timer(bool initstart) {
        if (initstart) {
            start();
        }
    }

    void start() { begin = std::chrono::steady_clock::now(); }

    double stop() {
        end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - begin).count();
    }

    double stop([[maybe_unused]] const std::string &msg) {
        double seconds = stop();
#if defined(SZ3_DEBUG_TIMINGS) && SZ3_DEBUG_TIMINGS
        std::cout << msg << " time = " << seconds << "s" << std::endl;
#endif
        return seconds;
    }

   private:
    std::chrono::time_point<std::chrono::steady_clock> begin, end;
};
}  // namespace SZ3

#endif  // SZ_TIMER_HPP
