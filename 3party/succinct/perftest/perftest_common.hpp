#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace succinct {
namespace detail {

    struct timer {
        timer()
            : m_tick(boost::posix_time::microsec_clock::universal_time())
            , m_done(false)
        {}

        bool done() { return m_done; }

        void report(double& elapsed) {
            elapsed = (double)(boost::posix_time::microsec_clock::universal_time() - m_tick).total_microseconds();
            m_done = true;
        }

        const std::string m_msg;
        boost::posix_time::ptime m_tick;
        bool m_done;
    };

}
}

#define SUCCINCT_TIMEIT(elapsed) \
    for (::succinct::detail::timer SUCCINCT_TIMEIT_timer;     \
         !SUCCINCT_TIMEIT_timer.done();          \
         SUCCINCT_TIMEIT_timer.report(elapsed)) \
        /**/
