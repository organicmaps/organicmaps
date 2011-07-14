//  boost/chrono/process_cpu_clocks.hpp  -----------------------------------------------------------//

//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/system for documentation.

#ifndef BOOST_CHRONO_PROCESS_CPU_CLOCKS_HPP
#define BOOST_CHRONO_PROCESS_CPU_CLOCKS_HPP

#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/system/error_code.hpp>
#include <boost/operators.hpp>
#include <boost/chrono/detail/system.hpp>
#include <iostream>


#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_prefix.hpp> // must be the last #include
#endif

namespace boost { namespace chrono {

    class BOOST_CHRONO_DECL process_real_cpu_clock {
    public:
        typedef nanoseconds                          duration;
        typedef duration::rep                        rep;
        typedef duration::period                     period;
        typedef chrono::time_point<process_real_cpu_clock>    time_point;
        BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =             true;

        static BOOST_CHRONO_INLINE time_point now(
                system::error_code & ec = BOOST_CHRONO_THROWS );
    };

    class BOOST_CHRONO_DECL process_user_cpu_clock {
    public:
        typedef nanoseconds                          duration;
        typedef duration::rep                        rep;
        typedef duration::period                     period;
        typedef chrono::time_point<process_user_cpu_clock>    time_point;
        BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =             true;

        static BOOST_CHRONO_INLINE time_point now( 
                system::error_code & ec = BOOST_CHRONO_THROWS );
    };

    class BOOST_CHRONO_DECL process_system_cpu_clock {
    public:
        typedef nanoseconds                          duration;
        typedef duration::rep                        rep;
        typedef duration::period                     period;
        typedef chrono::time_point<process_system_cpu_clock>    time_point;
        BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =             true;

        static BOOST_CHRONO_INLINE time_point now( 
                system::error_code & ec = BOOST_CHRONO_THROWS );
    };

        struct process_cpu_clock_times 
            : arithmetic<process_cpu_clock_times, 
            multiplicative<process_cpu_clock_times, process_real_cpu_clock::rep, 
            less_than_comparable<process_cpu_clock_times> > >
        {
            typedef process_real_cpu_clock::rep rep;
            process_cpu_clock_times()
                : real(0)
                , user(0)
                , system(0){}
            process_cpu_clock_times(
                process_real_cpu_clock::rep r,
                process_user_cpu_clock::rep   u,
                process_system_cpu_clock::rep s)
                : real(r)
                , user(u)
                , system(s){}

            process_real_cpu_clock::rep   real;    // real (i.e wall clock) time
            process_user_cpu_clock::rep   user;    // user cpu time
            process_system_cpu_clock::rep system;  // system cpu time

            bool operator==(process_cpu_clock_times const& rhs) {
                return (real==rhs.real &&
                        user==rhs.user &&
                        system==rhs.system);
            }

            process_cpu_clock_times operator+=(
                    process_cpu_clock_times const& rhs) 
            {
                real+=rhs.real;
                user+=rhs.user;
                system+=rhs.system;
                return *this;
            }
            process_cpu_clock_times operator-=(
                    process_cpu_clock_times const& rhs) 
            {
                real-=rhs.real;
                user-=rhs.user;
                system-=rhs.system;
                return *this;
            }
            process_cpu_clock_times operator*=(
                    process_cpu_clock_times const& rhs) 
            {
                real*=rhs.real;
                user*=rhs.user;
                system*=rhs.system;
                return *this;
            }
            process_cpu_clock_times operator*=(rep const& rhs) 
            {
                real*=rhs;
                user*=rhs;
                system*=rhs;
                return *this;
            }
            process_cpu_clock_times operator/=(process_cpu_clock_times const& rhs) 
            {
                real/=rhs.real;
                user/=rhs.user;
                system/=rhs.system;
                return *this;
            }
            process_cpu_clock_times operator/=(rep const& rhs) 
            {
                real/=rhs;
                user/=rhs;
                system/=rhs;
                return *this;
            }
            bool operator<(process_cpu_clock_times const & rhs) const 
            {
                if (real < rhs.real) return true;
                if (real > rhs.real) return false;
                if (user < rhs.user) return true;
                if (user > rhs.user) return false;
                if (system < rhs.system) return true;
                else return false;
            }

            template <class CharT, class Traits>
            void print(std::basic_ostream<CharT, Traits>& os) const 
            {
                os <<  "{"<< real <<";"<< user <<";"<< system << "}";
            }

            template <class CharT, class Traits>
            void read(std::basic_istream<CharT, Traits>& is) const 
            {
                typedef std::istreambuf_iterator<CharT, Traits> in_iterator;
                in_iterator i(is);
                in_iterator e;
                if (i == e || *i != '{')  // mandatory '{'
                {
                    is.setstate(is.failbit | is.eofbit);
                    return;
                }
                CharT x,y,z;
                is >> real >> x >> user >> y >> system >> z;
                if (!is.good() || (x != ';')|| (y != ';')|| (z != '}'))
                {
                    is.setstate(is.failbit);
                }
            }
        };

    class BOOST_CHRONO_DECL process_cpu_clock
    {
    public:

        typedef process_cpu_clock_times times;
        typedef boost::chrono::duration<times,  nano>                duration;
        typedef duration::rep                       rep;
        typedef duration::period                    period;
        typedef chrono::time_point<process_cpu_clock>  time_point;
        BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =           true;

        static BOOST_CHRONO_INLINE time_point now( 
                system::error_code & ec = BOOST_CHRONO_THROWS );
    };

    template <class CharT, class Traits>
    std::basic_ostream<CharT, Traits>& 
    operator<<(std::basic_ostream<CharT, Traits>& os, 
            process_cpu_clock_times const& rhs) 
    {
        rhs.print(os);
        return os;
    }

    template <class CharT, class Traits>
    std::basic_istream<CharT, Traits>& 
    operator>>(std::basic_istream<CharT, Traits>& is, 
            process_cpu_clock_times const& rhs) 
    {
        rhs.read(is);
        return is;
    }

    template <>
    struct duration_values<process_cpu_clock_times>
    {
        typedef process_cpu_clock_times Rep;
    public:
        static Rep zero() 
        {
            return Rep();
        }
        static Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()  
        {
          return Rep((std::numeric_limits<process_real_cpu_clock::rep>::max)(),
                      (std::numeric_limits<process_user_cpu_clock::rep>::max)(),
                      (std::numeric_limits<process_system_cpu_clock::rep>::max)());
        }
        static Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()  
        {
          return Rep((std::numeric_limits<process_real_cpu_clock::rep>::min)(),
                      (std::numeric_limits<process_user_cpu_clock::rep>::min)(),
                      (std::numeric_limits<process_system_cpu_clock::rep>::min)());
        }
    };

} // namespace chrono
} // namespace boost

namespace std {

    template <>
    class numeric_limits<boost::chrono::process_cpu_clock::times>
    {
        typedef boost::chrono::process_cpu_clock::times Rep;

        public:
        static const bool is_specialized = true;
        static Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()  
        {
          return Rep((std::numeric_limits<boost::chrono::process_real_cpu_clock::rep>::min)(),
                      (std::numeric_limits<boost::chrono::process_user_cpu_clock::rep>::min)(),
                      (std::numeric_limits<boost::chrono::process_system_cpu_clock::rep>::min)());
        }
        static Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()  
        {
          return Rep((std::numeric_limits<boost::chrono::process_real_cpu_clock::rep>::max)(),
                      (std::numeric_limits<boost::chrono::process_user_cpu_clock::rep>::max)(),
                      (std::numeric_limits<boost::chrono::process_system_cpu_clock::rep>::max)());
        }
        static Rep lowest() throw() 
        { 
            return (min)(); 
        }
        static const int digits = std::numeric_limits<boost::chrono::process_real_cpu_clock::rep>::digits+
                        std::numeric_limits<boost::chrono::process_user_cpu_clock::rep>::digits+
                        std::numeric_limits<boost::chrono::process_system_cpu_clock::rep>::digits;
        static const int digits10 = std::numeric_limits<boost::chrono::process_real_cpu_clock::rep>::digits10+
                        std::numeric_limits<boost::chrono::process_user_cpu_clock::rep>::digits10+
                        std::numeric_limits<boost::chrono::process_system_cpu_clock::rep>::digits10;
        //~ static const int max_digits10 = std::numeric_limits<boost::chrono::process_real_cpu_clock::rep>::max_digits10+
                        //~ std::numeric_limits<boost::chrono::process_user_cpu_clock::rep>::max_digits10+
                        //~ std::numeric_limits<boost::chrono::process_system_cpu_clock::rep>::max_digits10;
        static const bool is_signed = false;
        static const bool is_integer = true;
        static const bool is_exact = true;
        static const int radix = 0;
        //~ static Rep epsilon() throw() { return 0; }
        //~ static Rep round_error() throw() { return 0; }
        //~ static const int min_exponent = 0;
        //~ static const int min_exponent10 = 0;
        //~ static const int max_exponent = 0;
        //~ static const int max_exponent10 = 0;
        //~ static const bool has_infinity = false;
        //~ static const bool has_quiet_NaN = false;
        //~ static const bool has_signaling_NaN = false;
        //~ static const float_denorm_style has_denorm = denorm_absent;
        //~ static const bool has_denorm_loss = false;
        //~ static Rep infinity() throw() { return 0; }
        //~ static Rep quiet_NaN() throw() { return 0; }
        //~ static Rep signaling_NaN() throw() { return 0; }
        //~ static Rep denorm_min() throw() { return 0; }
        //~ static const bool is_iec559 = false;
        //~ static const bool is_bounded = true;
        //~ static const bool is_modulo = false;
        //~ static const bool traps = false;
        //~ static const bool tinyness_before = false;
        //~ static const float_round_style round_style = round_toward_zero;

    };
}

#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#else
#include <boost/chrono/detail/inlined/process_cpu_clocks.hpp>
#endif

#endif  // BOOST_CHRONO_PROCESS_CPU_CLOCKS_HPP
