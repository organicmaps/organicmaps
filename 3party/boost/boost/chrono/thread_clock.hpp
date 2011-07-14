//  boost/chrono/process_cpu_clocks.hpp  -----------------------------------------------------------//

//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/system for documentation.

#ifndef BOOST_CHRONO_THREAD_CLOCK_HPP
#define BOOST_CHRONO_THREAD_CLOCK_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/system/error_code.hpp>
#include <boost/chrono/detail/system.hpp>

#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_prefix.hpp> // must be the last #include
#endif

#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)

namespace boost { namespace chrono {

class BOOST_CHRONO_DECL thread_clock {
public:
    typedef nanoseconds                          duration;
    typedef duration::rep                        rep;
    typedef duration::period                     period;
    typedef chrono::time_point<thread_clock>    time_point;
    BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =             BOOST_CHRONO_THREAD_CLOCK_IS_STEADY;

    static BOOST_CHRONO_INLINE time_point now( );
    static BOOST_CHRONO_INLINE time_point now( system::error_code & ec );
};
} // namespace chrono
} // namespace boost

#endif

#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#else
#include <boost/chrono/detail/inlined/thread_clock.hpp>
#endif

#endif  // BOOST_CHRONO_THREAD_CLOCK_HPP
