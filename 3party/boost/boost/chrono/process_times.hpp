//  boost/chrono/process_times.hpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 1994, 2007, 2008
//  Copyright Vicente J Botet Escriba 2009-2010

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/system for documentation.

#ifndef BOOST_PROCESS_TIMES_HPP
#define BOOST_PROCESS_TIMES_HPP

#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/system/error_code.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <ostream>
#include <boost/chrono/detail/system.hpp>

#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_prefix.hpp> // must be the last #include
#endif


namespace boost
{
namespace chrono
{
//--------------------------------------------------------------------------------------//
//                                  process_clock                                       //
//--------------------------------------------------------------------------------------//

    class BOOST_CHRONO_DECL process_clock
    {
    public:
        typedef nanoseconds                          duration;
        typedef duration::rep                        rep;
        typedef duration::period                     period;
        typedef chrono::time_point<process_clock>    time_point;
        BOOST_CHRONO_STATIC_CONSTEXPR bool is_steady =             true;

        struct durations
        {
          process_clock::duration                       real;    // real (i.e wall clock) time
          process_clock::duration                       user;    // user cpu time
          process_clock::duration                       system;  // system cpu time
        };
        struct time_points
        {
          process_clock::time_point                       real;    // real (i.e wall clock) time
          process_clock::time_point                       user;    // user cpu time
          process_clock::time_point                       system;  // system cpu time
        };

        static BOOST_CHRONO_INLINE void now( durations & times,
                         system::error_code & ec = BOOST_CHRONO_THROWS );
        static BOOST_CHRONO_INLINE void now( time_points & times,
                         system::error_code & ec = BOOST_CHRONO_THROWS );
    };


//--------------------------------------------------------------------------------------//
//                                  process_times                                       //
//--------------------------------------------------------------------------------------//

    typedef process_clock::durations process_times;

//--------------------------------------------------------------------------------------//
//                                  process_timer                                       //
//--------------------------------------------------------------------------------------//

    class BOOST_CHRONO_DECL process_timer
    // BOOST_CHRONO_DECL is required to quiet compiler warnings even though
    // process_timer has no dynamically linked members, because process_timer is
    // used as a base class for run_timer, which does have dynamically linked members.
    {
    public:

      typedef process_clock                          clock;
      typedef process_clock::duration                duration;
      typedef process_clock::time_point              time_point;

      explicit process_timer( system::error_code & ec = BOOST_CHRONO_THROWS )
      {
        start(ec);
      }

     ~process_timer() {}  // never throws()

      void  start( system::error_code & ec = BOOST_CHRONO_THROWS )
      {
        process_clock::now( m_start, ec );
      }

      void  elapsed( process_times & times, system::error_code & ec = BOOST_CHRONO_THROWS )
      {
        process_times end;
        process_clock::now( end, ec );
        times.real  = end.real - m_start.real;
        times.user       = end.user - m_start.user;
        times.system     = end.system - m_start.system;
      }

    protected:
      process_times   m_start;
    private:
      process_timer(const process_timer&); // = delete;
      process_timer& operator=(const process_timer&); // = delete;
    };

//--------------------------------------------------------------------------------------//
//                                    run_timer                                         //
//--------------------------------------------------------------------------------------//

    class BOOST_CHRONO_DECL run_timer : public process_timer
    {
      // every function making use of inlined functions of class string are not inlined to avoid DLL issues
    public:

      // each constructor form has two overloads to avoid a visible default to
      // std::cout, which in turn would require including <iostream>, with its
      // high associated cost, even when the standard streams are not used.

      BOOST_CHRONO_INLINE
      explicit run_timer( system::error_code & ec = BOOST_CHRONO_THROWS );
      BOOST_CHRONO_INLINE
      explicit run_timer( std::ostream & os,
        system::error_code & ec = BOOST_CHRONO_THROWS );

      BOOST_CHRONO_INLINE
      explicit run_timer( const std::string & format,
        system::error_code & ec = BOOST_CHRONO_THROWS );
      BOOST_CHRONO_INLINE
      run_timer( std::ostream & os, const std::string & format,
        system::error_code & ec = BOOST_CHRONO_THROWS );

      BOOST_CHRONO_INLINE
      run_timer( const std::string & format, int places,
        system::error_code & ec = BOOST_CHRONO_THROWS );
      BOOST_CHRONO_INLINE
      run_timer( std::ostream & os, const std::string & format,
        int places, system::error_code & ec = BOOST_CHRONO_THROWS );

      BOOST_CHRONO_INLINE
      explicit run_timer( int places,
        system::error_code & ec = BOOST_CHRONO_THROWS );
      BOOST_CHRONO_INLINE
      run_timer( std::ostream & os, int places,
        system::error_code & ec = BOOST_CHRONO_THROWS );

      BOOST_CHRONO_INLINE
      run_timer( int places, const std::string & format,
        system::error_code & ec = BOOST_CHRONO_THROWS );
      BOOST_CHRONO_INLINE
      run_timer( std::ostream & os, int places, const std::string & format,
        system::error_code & ec = BOOST_CHRONO_THROWS );

      ~run_timer()  // never throws
      {
        system::error_code ec;
        if ( !reported() ) report( ec );
      }

      BOOST_CHRONO_INLINE void  start( system::error_code & ec = BOOST_CHRONO_THROWS )
      {
        m_reported = false;
        process_timer::start( ec );
      }

      BOOST_CHRONO_INLINE void  report( system::error_code & ec = BOOST_CHRONO_THROWS );

      BOOST_CHRONO_INLINE void  test_report( duration real_, duration user_, duration system_ );

      BOOST_CHRONO_INLINE bool  reported() const { return m_reported; }

      BOOST_CHRONO_INLINE static int default_places() { return m_default_places; }

    private:
      int             m_places;
      std::ostream &  m_os;

#if defined _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
      std::string     m_format;
#if defined _MSC_VER
#pragma warning(pop)
#endif
      bool            m_reported;

      BOOST_CHRONO_INLINE static std::ostream &  m_cout();
      //{return std::cout;}
      static const int m_default_places = 3;
      run_timer(const run_timer&); // = delete;
      run_timer& operator=(const run_timer&); // = delete;
    };

  } // namespace chrono
} // namespace boost

#ifndef BOOST_CHRONO_HEADER_ONLY
#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#else
#include <boost/chrono/detail/inlined/process_clock.hpp>
#include <boost/chrono/detail/inlined/run_timer.hpp>
#include <boost/chrono/detail/inlined/run_timer_static.hpp>
#endif

#endif  // BOOST_PROCESS_TIMES_HPP
