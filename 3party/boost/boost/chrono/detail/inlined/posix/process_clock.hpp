//  boost process_timer.cpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 1994, 2006, 2008
//  Copyright Vicente J. Botet Escriba 2009

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//

#include <boost/chrono/config.hpp>
#include <boost/chrono/process_times.hpp>
#include <boost/assert.hpp>

# include <sys/times.h>
# include <unistd.h>


namespace boost { namespace chrono {
namespace chrono_detail
{
  inline long tick_factor()        // multiplier to convert ticks
                            //  to nanoseconds; -1 if unknown
  {
    static long factor = 0;
    if ( !factor )
    {
      if ( (factor = ::sysconf( _SC_CLK_TCK )) <= 0 )
        factor = -1;
      else
      {
        BOOST_ASSERT( factor <= 1000000l ); // doesn't handle large ticks
        factor = 1000000l / factor;  // compute factor
        if ( !factor ) factor = -1;
      }
    }
    return factor;
  }
}

    void process_clock::now( process_times & times_, system::error_code & ec ) {

        tms tm;
        clock_t c = ::times( &tm );
        if ( c == clock_t(-1) ) // error
        {
            if (BOOST_CHRONO_IS_THROWS(ec))
            {
                boost::throw_exception(
                        system::system_error( 
                                errno, 
                                BOOST_CHRONO_SYSTEM_CATEGORY, 
                                "chrono::process_clock" ));
            }
            else
            {
                ec.assign( errno, BOOST_CHRONO_SYSTEM_CATEGORY );
                times_.real = times_.system = times_.user = nanoseconds(-1);
            }
        }
        else
        {
            times_.real = microseconds(c);
            times_.system = microseconds(tm.tms_stime + tm.tms_cstime);
            times_.user = microseconds(tm.tms_utime + tm.tms_cutime);
            if ( chrono_detail::tick_factor() != -1 )
            {
                if (!BOOST_CHRONO_IS_THROWS(ec)) 
                {
                  ec.clear();
                }
                times_.real *= chrono_detail::tick_factor();
                times_.user *= chrono_detail::tick_factor();
                times_.system *= chrono_detail::tick_factor();
            }
            else
            {
                if (BOOST_CHRONO_IS_THROWS(ec))
                {
                    boost::throw_exception(
                            system::system_error( 
                                    errno, 
                                    BOOST_CHRONO_SYSTEM_CATEGORY, 
                                    "chrono::process_clock" ));
                }
                else
                {
                    ec.assign( errno, BOOST_CHRONO_SYSTEM_CATEGORY );
                    times_.real = times_.user = times_.system = nanoseconds(-1);
                }
            }
        }

    }
} }
