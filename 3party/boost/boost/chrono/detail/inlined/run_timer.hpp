//  boost run_timer.cpp  ---------------------------------------------------------------//

//  Copyright Beman Dawes 1994, 2006, 2008
//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//
#ifndef BOOST_CHRONO_DETAIL_INLINED_RUN_TIMER_HPP
#define BOOST_CHRONO_DETAIL_INLINED_RUN_TIMER_HPP

#include <boost/version.hpp>
#include <boost/chrono/process_times.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/io/ios_state.hpp>
#include <cstring>
#include <boost/assert.hpp>


namespace boost
{
namespace chrono
{
namespace chrono_detail
{
  BOOST_CHRONO_INLINE
  const char * default_format() {
    return "\nreal %rs, cpu %cs (%p%), user %us, system %ss\n";
  }

  BOOST_CHRONO_INLINE
  void show_time( const boost::chrono::process_times & times,
                  const char * format, int places, std::ostream & os )
  //  NOTE WELL: Will truncate least-significant digits to LDBL_DIG, which may
  //  be as low as 10, although will be 15 for many common platforms.
  {
    if ( times.real < nanoseconds(0) ) return;
    if ( places > 9 )
      places = 9;  // sanity check
    else if ( places < 0 )
      places = 0;

    boost::io::ios_flags_saver ifs( os );
    os.setf( std::ios_base::fixed, std::ios_base::floatfield );
    boost::io::ios_precision_saver ips( os );
    os.precision( places );

    nanoseconds total = times.system + times.user;

    for ( ; *format; ++format )
    {
      if ( *format != '%' || !*(format+1) || !std::strchr("rcpus", *(format+1)) )
        os << *format;
      else
      {
        ++format;
        switch ( *format )
        {
        case 'r':
          os << duration<double>(times.real).count();
          break;
        case 'u':
          os << duration<double>(times.user).count();
          break;
        case 's':
          os << duration<double>(times.system).count();
          break;
        case 'c':
          os << duration<double>(total).count();
          break;
        case 'p':
          {
            boost::io::ios_precision_saver ips( os );
            os.precision( 1 );
            if ( times.real.count() && total.count() )
              os << duration<double>(total).count()
                   /duration<double>(times.real).count() * 100.0;
            else
              os << 0.0;
          }
          break;
        default:
          BOOST_ASSERT(0 && "run_timer internal logic error");
        }
      }
    }
  }

}



      run_timer::run_timer( system::error_code & ec  )
        : m_places(m_default_places), m_os(m_cout()) { start(ec); }
      run_timer::run_timer( std::ostream & os,
        system::error_code & ec  )
        : m_places(m_default_places), m_os(os) { start(ec); }

      run_timer::run_timer( const std::string & format,
        system::error_code & ec  )
        : m_places(m_default_places), m_os(m_cout()), m_format(format) { start(ec); }
      run_timer::run_timer( std::ostream & os, const std::string & format,
        system::error_code & ec  )
        : m_places(m_default_places), m_os(os), m_format(format) { start(ec); }

      run_timer::run_timer( const std::string & format, int places,
        system::error_code & ec  )
        : m_places(places), m_os(m_cout()), m_format(format) { start(ec); }
      run_timer::run_timer( std::ostream & os, const std::string & format,
        int places, system::error_code & ec  )
        : m_places(places), m_os(os), m_format(format) { start(ec); }

      run_timer::run_timer( int places,
        system::error_code & ec  )
        : m_places(places), m_os(m_cout()) { start(ec); }
      run_timer::run_timer( std::ostream & os, int places,
        system::error_code & ec  )
        : m_places(places), m_os(os) { start(ec); }

      run_timer::run_timer( int places, const std::string & format,
        system::error_code & ec  )
        : m_places(places), m_os(m_cout()), m_format(format) { start(ec); }
      run_timer::run_timer( std::ostream & os, int places, const std::string & format,
        system::error_code & ec  )
        : m_places(places), m_os(os), m_format(format) { start(ec); }

    //  run_timer::report  -------------------------------------------------------------//

    void run_timer::report( system::error_code & ec )
    {
      m_reported = true;
      if ( m_format.empty() ) m_format = chrono_detail::default_format();

      process_times times;
      elapsed( times, ec );
      if (ec) return;

      if ( BOOST_CHRONO_IS_THROWS(ec) )
      {
        chrono_detail::show_time( times, m_format.c_str(), m_places, m_os );
      }
      else // non-throwing
      {
        try
        {
          chrono_detail::show_time( times, m_format.c_str(), m_places, m_os );
          if (!BOOST_CHRONO_IS_THROWS(ec)) 
          {
            ec.clear();
          }
        }

        catch (...) // eat any exceptions
        {
          BOOST_ASSERT( 0 && "error reporting not fully implemented yet" );
          if (BOOST_CHRONO_IS_THROWS(ec))
          {
              boost::throw_exception(
                      system::system_error( 
                              errno, 
                              BOOST_CHRONO_SYSTEM_CATEGORY, 
                              "chrono::run_timer" ));
          } 
          else
          {
            ec.assign(system::errc::success, BOOST_CHRONO_SYSTEM_CATEGORY);
          }
          }
      }
    }

    //  run_timer::test_report  --------------------------------------------------------//

    void run_timer::test_report( duration real_, duration user_, duration system_ )
    {
      if ( m_format.empty() ) m_format = chrono_detail::default_format();

      process_times times;
      times.real = real_;
      times.user = user_;
      times.system = system_;

      chrono_detail::show_time( times, m_format.c_str(), m_places, m_os );
    }

  } // namespace chrono
} // namespace boost

#endif
