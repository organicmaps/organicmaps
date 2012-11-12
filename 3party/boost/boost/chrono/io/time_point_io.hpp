//  (C) Copyright Howard Hinnant
//  (C) Copyright 2010-2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

//===-------------------------- locale ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This code was adapted by Vicente from Howard Hinnant's experimental work
// on chrono i/o to Boost and some functions from libc++/locale to emulate the missing time_get::get()

#ifndef BOOST_CHRONO_IO_TIME_POINT_IO_HPP
#define BOOST_CHRONO_IO_TIME_POINT_IO_HPP

#include <boost/chrono/io/time_point_put.hpp>
#include <boost/chrono/io/time_point_get.hpp>
#include <boost/chrono/io/duration_io.hpp>
#include <boost/chrono/io/ios_base_state.hpp>
#include <boost/chrono/io/utility/manip_base.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/clock_string.hpp>
#include <boost/chrono/round.hpp>
#include <boost/chrono/detail/scan_keyword.hpp>
#include <cstring>
#include <locale>
#include <string.h>

#define  BOOST_CHRONO_INTERNAL_TIMEGM defined BOOST_WINDOWS && ! defined(__CYGWIN__)
//#define  BOOST_CHRONO_INTERNAL_TIMEGM 1

#define  BOOST_CHRONO_USES_INTERNAL_TIME_GET

namespace boost
{
  namespace chrono
  {
    namespace detail
    {

      template <class CharT, class InputIterator = std::istreambuf_iterator<CharT> >
      struct time_get
      {
        std::time_get<CharT> const &that_;
        time_get(std::time_get<CharT> const& that) : that_(that) {}

        typedef std::time_get<CharT> facet;
        typedef typename facet::iter_type iter_type;
        typedef typename facet::char_type char_type;
        typedef std::basic_string<char_type> string_type;

        static int
        get_up_to_n_digits(
            InputIterator& b, InputIterator e,
            std::ios_base::iostate& err,
            const std::ctype<CharT>& ct,
            int n)
        {
            // Precondition:  n >= 1
            if (b == e)
            {
                err |= std::ios_base::eofbit | std::ios_base::failbit;
                return 0;
            }
            // get first digit
            CharT c = *b;
            if (!ct.is(std::ctype_base::digit, c))
            {
                err |= std::ios_base::failbit;
                return 0;
            }
            int r = ct.narrow(c, 0) - '0';
            for (++b, --n; b != e && n > 0; ++b, --n)
            {
                // get next digit
                c = *b;
                if (!ct.is(std::ctype_base::digit, c))
                    return r;
                r = r * 10 + ct.narrow(c, 0) - '0';
            }
            if (b == e)
                err |= std::ios_base::eofbit;
            return r;
        }


        void get_day(
            int& d,
            iter_type& b, iter_type e,
            std::ios_base::iostate& err,
            const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && 1 <= t && t <= 31)
                d = t;
            else
                err |= std::ios_base::failbit;
        }

        void get_month(
            int& m,
            iter_type& b, iter_type e,
            std::ios_base::iostate& err,
            const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2) - 1;
            if (!(err & std::ios_base::failbit) && t <= 11)
                m = t;
            else
                err |= std::ios_base::failbit;
        }


        void get_year4(int& y,
                                                      iter_type& b, iter_type e,
                                                      std::ios_base::iostate& err,
                                                      const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 4);
            if (!(err & std::ios_base::failbit))
                y = t - 1900;
        }

        void
        get_hour(int& h,
                                                     iter_type& b, iter_type e,
                                                     std::ios_base::iostate& err,
                                                     const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 23)
                h = t;
            else
                err |= std::ios_base::failbit;
        }

        void
        get_minute(int& m,
                                                       iter_type& b, iter_type e,
                                                       std::ios_base::iostate& err,
                                                       const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 59)
                m = t;
            else
                err |= std::ios_base::failbit;
        }

        void
        get_second(int& s,
                                                       iter_type& b, iter_type e,
                                                       std::ios_base::iostate& err,
                                                       const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 60)
                s = t;
            else
                err |= std::ios_base::failbit;
        }



        InputIterator get(
            iter_type b, iter_type e,
            std::ios_base& iob,
            std::ios_base::iostate& err,
            std::tm* tm,
            char fmt, char) const
        {
            err = std::ios_base::goodbit;
            const std::ctype<char_type>& ct = std::use_facet<std::ctype<char_type> >(iob.getloc());

            switch (fmt)
            {
//            case 'a':
//            case 'A':
//                that_.get_weekdayname(tm->tm_wday, b, e, err, ct);
//                break;
//            case 'b':
//            case 'B':
//            case 'h':
//              that_.get_monthname(tm->tm_mon, b, e, err, ct);
//                break;
//            case 'c':
//                {
//                const string_type& fm = this->c();
//                b = that_.get(b, e, iob, err, tm, fm.data(), fm.data() + fm.size());
//                }
//                break;
            case 'd':
            case 'e':
              get_day(tm->tm_mday, b, e, err, ct);
              //std::cerr << "tm_mday= "<< tm->tm_mday << std::endl;

                break;
//            case 'D':
//                {
//                const char_type fm[] = {'%', 'm', '/', '%', 'd', '/', '%', 'y'};
//                b = that_.get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
//                }
//                break;
//            case 'F':
//                {
//                const char_type fm[] = {'%', 'Y', '-', '%', 'm', '-', '%', 'd'};
//                b = that_.get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
//                }
//                break;
            case 'H':
              get_hour(tm->tm_hour, b, e, err, ct);
              //std::cerr << "tm_hour= "<< tm->tm_hour << std::endl;
                break;
//            case 'I':
//              that_.get_12_hour(tm->tm_hour, b, e, err, ct);
//                break;
//            case 'j':
//              that_.get_day_year_num(tm->tm_yday, b, e, err, ct);
//                break;
            case 'm':
              get_month(tm->tm_mon, b, e, err, ct);
              //std::cerr << "tm_mon= "<< tm->tm_mon << std::endl;
                break;
            case 'M':
              get_minute(tm->tm_min, b, e, err, ct);
              //std::cerr << "tm_min= "<< tm->tm_min << std::endl;
                break;
//            case 'n':
//            case 't':
//              that_.get_white_space(b, e, err, ct);
//                break;
//            case 'p':
//              that_.get_am_pm(tm->tm_hour, b, e, err, ct);
//                break;
//            case 'r':
//                {
//                const char_type fm[] = {'%', 'I', ':', '%', 'M', ':', '%', 'S', ' ', '%', 'p'};
//                b = that_.get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
//                }
//                break;
//            case 'R':
//                {
//                const char_type fm[] = {'%', 'H', ':', '%', 'M'};
//                b = that_.get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
//                }
//                break;
//            case 'S':
//              that_.get_second(tm->tm_sec, b, e, err, ct);
//                break;
//            case 'T':
//                {
//                const char_type fm[] = {'%', 'H', ':', '%', 'M', ':', '%', 'S'};
//                b = that_.get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
//                }
//                break;
//            case 'w':
//              that_.get_weekday(tm->tm_wday, b, e, err, ct);
//                break;
//            case 'x':
//                return that_.get_date(b, e, iob, err, tm);
//            case 'X':
//                {
//                const string_type& fm = this->X();
//                b = that_.get(b, e, iob, err, tm, fm.data(), fm.data() + fm.size());
//                }
//                break;
//            case 'y':
//              that_.get_year(tm->tm_year, b, e, err, ct);
                break;
            case 'Y':
              get_year4(tm->tm_year, b, e, err, ct);
              //std::cerr << "tm_year= "<< tm->tm_year << std::endl;
                break;
//            case '%':
//              that_.get_percent(b, e, err, ct);
//                break;
            default:
                err |= std::ios_base::failbit;
            }
            return b;
        }


        InputIterator get(
          iter_type b, iter_type e,
          std::ios_base& iob,
          std::ios_base::iostate& err, std::tm* tm,
          const char_type* fmtb, const char_type* fmte) const
        {
          const std::ctype<char_type>& ct = std::use_facet<std::ctype<char_type> >(iob.getloc());
          err = std::ios_base::goodbit;
          while (fmtb != fmte && err == std::ios_base::goodbit)
          {
              if (b == e)
              {
                  err = std::ios_base::failbit;
                  break;
              }
              if (ct.narrow(*fmtb, 0) == '%')
              {
                  if (++fmtb == fmte)
                  {
                      err = std::ios_base::failbit;
                      break;
                  }
                  char cmd = ct.narrow(*fmtb, 0);
                  char opt = '\0';
                  if (cmd == 'E' || cmd == '0')
                  {
                      if (++fmtb == fmte)
                      {
                          err = std::ios_base::failbit;
                          break;
                      }
                      opt = cmd;
                      cmd = ct.narrow(*fmtb, 0);
                  }
                  b = get(b, e, iob, err, tm, cmd, opt);
                  ++fmtb;
              }
              else if (ct.is(std::ctype_base::space, *fmtb))
              {
                  for (++fmtb; fmtb != fmte && ct.is(std::ctype_base::space, *fmtb); ++fmtb)
                      ;
                  for (        ;    b != e    && ct.is(std::ctype_base::space, *b);    ++b)
                      ;
              }
              else if (ct.toupper(*b) == ct.toupper(*fmtb))
              {
                  ++b;
                  ++fmtb;
              }
              else
                  err = std::ios_base::failbit;
          }
          if (b == e)
              err |= std::ios_base::eofbit;
          return b;
        }

      };


      template <class CharT>
      class time_manip: public manip<time_manip<CharT> >
      {
        std::basic_string<CharT> fmt_;
        timezone tz_;
      public:

        time_manip(timezone tz, std::basic_string<CharT> fmt)
        // todo move semantics
        :
          fmt_(fmt), tz_(tz)
        {
        }

        /**
         * Change the timezone and time format ios state;
         */
        void operator()(std::ios_base &ios) const
        {
          set_time_fmt<CharT> (ios, fmt_);
          set_timezone(ios, tz_);
        }
      };

      class time_man: public manip<time_man>
      {
        timezone tz_;
      public:

        time_man(timezone tz)
        // todo move semantics
        :
          tz_(tz)
        {
        }

        /**
         * Change the timezone and time format ios state;
         */
        void operator()(std::ios_base &ios) const
        {
          //set_time_fmt<typename out_stream::char_type>(ios, "");
          set_timezone(ios, tz_);
        }
      };

    }

    template <class CharT>
    inline detail::time_manip<CharT> time_fmt(timezone tz, const CharT* fmt)
    {
      return detail::time_manip<CharT>(tz, fmt);
    }

    template <class CharT>
    inline detail::time_manip<CharT> time_fmt(timezone tz, std::basic_string<CharT> fmt)
    {
      // todo move semantics
      return detail::time_manip<CharT>(tz, fmt);
    }

    inline detail::time_man time_fmt(timezone f)
    {
      return detail::time_man(f);
    }

    /**
     * time_fmt_io_saver i/o saver.
     *
     * See Boost.IO i/o state savers for a motivating compression.
     */
    template <typename CharT = char, typename Traits = std::char_traits<CharT> >
    struct time_fmt_io_saver
    {

      //! the type of the state to restore
      typedef std::basic_ostream<CharT, Traits> state_type;
      //! the type of aspect to save
      typedef std::basic_string<CharT, Traits> aspect_type;

      /**
       * Explicit construction from an i/o stream.
       *
       * Store a reference to the i/o stream and the value of the associated @c time format .
       */
      explicit time_fmt_io_saver(state_type &s) :
        s_save_(s), a_save_(get_time_fmt(s_save_))
      {
      }

      /**
       * Construction from an i/o stream and a @c time format  to restore.
       *
       * Stores a reference to the i/o stream and the value @c new_value to restore given as parameter.
       */
      time_fmt_io_saver(state_type &s, aspect_type new_value) :
        s_save_(s), a_save_(new_value)
      {
      }

      /**
       * Destructor.
       *
       * Restores the i/o stream with the format to be restored.
       */
      ~time_fmt_io_saver()
      {
        this->restore();
      }

      /**
       * Restores the i/o stream with the time format to be restored.
       */
      void restore()
      {
        set_time_fmt(a_save_, a_save_);
      }
    private:
      state_type& s_save_;
      aspect_type a_save_;
    };

    /**
     * timezone_io_saver i/o saver.
     *
     * See Boost.IO i/o state savers for a motivating compression.
     */
    struct timezone_io_saver
    {

      //! the type of the state to restore
      typedef std::ios_base state_type;
      //! the type of aspect to save
      typedef timezone aspect_type;

      /**
       * Explicit construction from an i/o stream.
       *
       * Store a reference to the i/o stream and the value of the associated @c timezone.
       */
      explicit timezone_io_saver(state_type &s) :
        s_save_(s), a_save_(get_timezone(s_save_))
      {
      }

      /**
       * Construction from an i/o stream and a @c timezone to restore.
       *
       * Stores a reference to the i/o stream and the value @c new_value to restore given as parameter.
       */
      timezone_io_saver(state_type &s, aspect_type new_value) :
        s_save_(s), a_save_(new_value)
      {
      }

      /**
       * Destructor.
       *
       * Restores the i/o stream with the format to be restored.
       */
      ~timezone_io_saver()
      {
        this->restore();
      }

      /**
       * Restores the i/o stream with the timezone to be restored.
       */
      void restore()
      {
        set_timezone(s_save_, a_save_);
      }
    private:
      state_type& s_save_;
      aspect_type a_save_;
    };

    /**
     *
     * @param os
     * @param tp
     * @Effects Behaves as a formatted output function. After constructing a @c sentry object, if the @ sentry
     * converts to true, calls to @c facet.put(os,os,os.fill(),tp) where @c facet is the @c time_point_put<CharT>
     * facet associated to @c os or a new created instance of the default @c time_point_put<CharT> facet.
     * @return @c os.
     */
    template <class CharT, class Traits, class Clock, class Duration>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const time_point<Clock, Duration>& tp)
    {

      typedef std::basic_string<CharT, Traits> string_type;
#ifndef BOOST_NO_EXCEPTIONS
      bool failed = false;
      try // BOOST_NO_EXCEPTIONS protected
#endif
      {
        std::ios_base::iostate err = std::ios_base::goodbit;
#ifndef BOOST_NO_EXCEPTIONS
        try // BOOST_NO_EXCEPTIONS protected
#endif
        {
          typename std::basic_ostream<CharT, Traits>::sentry opfx(os);
          if (bool(opfx))
          {
            if (!std::has_facet<time_point_put<CharT> >(os.getloc()))
            {
              if (time_point_put<CharT> ().put(os, os, os.fill(), tp) .failed())
              {
                err = std::ios_base::badbit;
              }
            }
            else
            {
              if (std::use_facet<time_point_put<CharT> >(os.getloc()) .put(os, os, os.fill(), tp).failed())
              {
                err = std::ios_base::badbit;
              }
            }
            os.width(0);
          }
        }
#ifndef BOOST_NO_EXCEPTIONS
        catch (...) // BOOST_NO_EXCEPTIONS protected
        {
          bool flag = false;
          try // BOOST_NO_EXCEPTIONS protected
          {
            os.setstate(std::ios_base::failbit);
          }
          catch (std::ios_base::failure ) // BOOST_NO_EXCEPTIONS protected
          {
            flag = true;
          }
          if (flag) throw;
        }
#endif
        if (err) os.setstate(err);
        return os;
      }
#ifndef BOOST_NO_EXCEPTIONS
      catch (...) // BOOST_NO_EXCEPTIONS protected
      {
        failed = true;
      }
      if (failed) os.setstate(std::ios_base::failbit | std::ios_base::badbit);
#endif
      return os;
    }

    template <class CharT, class Traits, class Clock, class Duration>
    std::basic_istream<CharT, Traits>&
    operator>>(std::basic_istream<CharT, Traits>& is, time_point<Clock, Duration>& tp)
    {
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
      std::ios_base::iostate err = std::ios_base::goodbit;

#ifndef BOOST_NO_EXCEPTIONS
      try // BOOST_NO_EXCEPTIONS protected
#endif
      {
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        typename std::basic_istream<CharT, Traits>::sentry ipfx(is);
        if (bool(ipfx))
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          if (!std::has_facet<time_point_get<CharT> >(is.getloc()))
          {
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            time_point_get<CharT> ().get(is, std::istreambuf_iterator<CharT, Traits>(), is, err, tp);
          }
          else
          {
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            std::use_facet<time_point_get<CharT> >(is.getloc()).get(is, std::istreambuf_iterator<CharT, Traits>(), is,
                err, tp);
          }
        }
      }
#ifndef BOOST_NO_EXCEPTIONS
      catch (...) // BOOST_NO_EXCEPTIONS protected
      {
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        bool flag = false;
        try
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          is.setstate(std::ios_base::failbit);
        }
        catch (std::ios_base::failure ) // BOOST_NO_EXCEPTIONS protected
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          flag = true;
        }
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        if (flag) throw;
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
      }
#endif
      if (err) is.setstate(err);
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
      return is;
    }

#ifndef BOOST_CHRONO_NO_UTC_TIMEPOINT

    namespace detail
    {
#if BOOST_CHRONO_INTERNAL_TIMEGM
    int is_leap(int year)
    {
      if(year % 400 == 0)
      return 1;
      if(year % 100 == 0)
      return 0;
      if(year % 4 == 0)
      return 1;
      return 0;
    }
    inline int days_from_0(int year)
    {
      year--;
      return 365 * year + (year / 400) - (year/100) + (year / 4);
    }
    int days_from_1970(int year)
    {
      static const int days_from_0_to_1970 = days_from_0(1970);
      return days_from_0(year) - days_from_0_to_1970;
    }
    int days_from_1jan(int year,int month,int day)
    {
      static const int days[2][12] =
      {
        { 0,31,59,90,120,151,181,212,243,273,304,334},
        { 0,31,60,91,121,152,182,213,244,274,305,335}
      };
      return days[is_leap(year)][month-1] + day - 1;
    }

    time_t internal_timegm(std::tm const *t)
    {
      int year = t->tm_year + 1900;
      int month = t->tm_mon;
      if(month > 11)
      {
        year += month/12;
        month %= 12;
      }
      else if(month < 0)
      {
        int years_diff = (-month + 11)/12;
        year -= years_diff;
        month+=12 * years_diff;
      }
      month++;
      int day = t->tm_mday;
      int day_of_year = days_from_1jan(year,month,day);
      int days_since_epoch = days_from_1970(year) + day_of_year;

      time_t seconds_in_day = 3600 * 24;
      time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

      return result;
    }
#endif
    } // detail

#if defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT
    template <class CharT, class Traits, class Duration>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const time_point<system_clock, Duration>& tp)
    {
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
      typename std::basic_ostream<CharT, Traits>::sentry ok(os);
      if (bool(ok))
      {
        //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
        bool failed = false;
#ifndef BOOST_NO_EXCEPTIONS
        try // BOOST_NO_EXCEPTIONS protected
#endif
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          const CharT* pb = 0; //nullptr;
          const CharT* pe = pb;
          std::basic_string<CharT> fmt = get_time_fmt<CharT> (os);
          pb = fmt.data();
          pe = pb + fmt.size();

          timezone tz = get_timezone(os);
          std::locale loc = os.getloc();
          time_t t = system_clock::to_time_t(time_point_cast<system_clock::duration>(tp));
          std::tm tm;
          if (tz == timezone::local)
          {
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
#if defined BOOST_WINDOWS && ! defined(__CYGWIN__)
            std::tm *tmp = 0;
            if ((tmp=localtime(&t)) == 0)
            failed = true;
            tm =*tmp;
#else
            if (localtime_r(&t, &tm) == 0) failed = true;
#endif
          }
          else
          {
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
#if defined BOOST_WINDOWS && ! defined(__CYGWIN__)
            std::tm *tmp = 0;
            if((tmp = gmtime(&t)) == 0)
            failed = true;
            tm = *tmp;
#else
            if (gmtime_r(&t, &tm) == 0) failed = true;
#endif

          }
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          if (!failed)
          {
            //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            const std::time_put<CharT>& tpf = std::use_facet<std::time_put<CharT> >(loc);
            if (pb == pe)
            {
              //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
              CharT pattern[] =
              { '%', 'Y', '-', '%', 'm', '-', '%', 'd', ' ', '%', 'H', ':', '%', 'M', ':' };
              pb = pattern;
              pe = pb + sizeof (pattern) / sizeof(CharT);
              failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
              if (!failed)
              {
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                duration<double> d = tp - system_clock::from_time_t(t) + seconds(tm.tm_sec);
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                if (d.count() < 10) os << CharT('0');
                //if (! os.good()) {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                //  throw "exception";
                //}
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                std::ios::fmtflags flgs = os.flags();
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                os.setf(std::ios::fixed, std::ios::floatfield);
                //if (! os.good()) {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                //throw "exception";
                //}
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< " " << d.count()  << std::endl;
                os << d.count();
                //if (! os.good()) {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                //throw "exception";
                //}
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< " " << d.count() << std::endl;
                os.flags(flgs);
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                if (tz == timezone::local)
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                  CharT sub_pattern[] =
                  { ' ', '%', 'z' };
                  pb = sub_pattern;
                  pe = pb + +sizeof (sub_pattern) / sizeof(CharT);
                  failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
                }
                else
                {
                  //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
                  CharT sub_pattern[] =
                  { ' ', '+', '0', '0', '0', '0', 0 };
                  os << sub_pattern;
                }
                //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
              }
              //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
            }
            else
            {
              //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
              failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
            }
          }
        }
#ifndef BOOST_NO_EXCEPTIONS
        catch (...) // BOOST_NO_EXCEPTIONS protected
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          failed = true;
        }
#endif
        if (failed)
        {
          //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
          os.setstate(std::ios_base::failbit | std::ios_base::badbit);
        }
      }
      //std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
      return os;
    }
#endif

    namespace detail
    {

      template <class CharT, class InputIterator>
      minutes extract_z(InputIterator& b, InputIterator e, std::ios_base::iostate& err, const std::ctype<CharT>& ct)
      {
        int min = 0;
        if (b != e)
        {
          char cn = ct.narrow(*b, 0);
          if (cn != '+' && cn != '-')
          {
            err |= std::ios_base::failbit;
            return minutes(0);
          }
          int sn = cn == '-' ? -1 : 1;
          int hr = 0;
          for (int i = 0; i < 2; ++i)
          {
            if (++b == e)
            {
              err |= std::ios_base::eofbit | std::ios_base::failbit;
              return minutes(0);
            }
            cn = ct.narrow(*b, 0);
            if (! ('0' <= cn && cn <= '9'))
            {
              err |= std::ios_base::failbit;
              return minutes(0);
            }
            hr = hr * 10 + cn - '0';
          }
          for (int i = 0; i < 2; ++i)
          {
            if (++b == e)
            {
              err |= std::ios_base::eofbit | std::ios_base::failbit;
              return minutes(0);
            }
            cn = ct.narrow(*b, 0);
            if (! ('0' <= cn && cn <= '9'))
            {
              err |= std::ios_base::failbit;
              return minutes(0);
            }
            min = min * 10 + cn - '0';
          }
          if (++b == e) {
            err |= std::ios_base::eofbit;
          }
          min += hr * 60;
          min *= sn;
        }
        else
        {
          err |= std::ios_base::eofbit | std::ios_base::failbit;
        }
        return minutes(min);
      }

    } // detail

#if defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT
    template <class CharT, class Traits, class Duration>
    std::basic_istream<CharT, Traits>&
    operator>>(std::basic_istream<CharT, Traits>& is, time_point<system_clock, Duration>& tp)
    {
      typename std::basic_istream<CharT, Traits>::sentry ok(is);
      if (bool(ok))
      {
        std::ios_base::iostate err = std::ios_base::goodbit;
#ifndef BOOST_NO_EXCEPTIONS
        try
#endif
        {
          const CharT* pb = 0; //nullptr;
          const CharT* pe = pb;
          std::basic_string<CharT> fmt = get_time_fmt<CharT> (is);
          pb = fmt.data();
          pe = pb + fmt.size();

          timezone tz = get_timezone(is);
          std::locale loc = is.getloc();
          const std::time_get<CharT>& tg = std::use_facet<std::time_get<CharT> >(loc);
          const std::ctype<CharT>& ct = std::use_facet<std::ctype<CharT> >(loc);
          tm tm; // {0}
          typedef std::istreambuf_iterator<CharT, Traits> It;
          if (pb == pe)
          {
            CharT pattern[] =
            { '%', 'Y', '-', '%', 'm', '-', '%', 'd', ' ', '%', 'H', ':', '%', 'M', ':' };
            pb = pattern;
            pe = pb + sizeof (pattern) / sizeof(CharT);
#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
            const detail::time_get<CharT>& dtg(tg);
            dtg.get(is, 0, is, err, &tm, pb, pe);
#else
            tg.get(is, 0, is, err, &tm, pb, pe);
#endif
            if (err & std::ios_base::failbit) goto exit;
            double sec;
            CharT c = CharT();
            is >> sec;
            if (is.fail())
            {
              err |= std::ios_base::failbit;
              goto exit;
            }
            //std::cerr << "sec= "<< sec << std::endl;
            It i(is);
            It eof;
            c = *i;
            if (++i == eof || c != ' ')
            {
              err |= std::ios_base::failbit;
              goto exit;
            }
            minutes min = detail::extract_z(i, eof, err, ct);
            //std::cerr << "min= "<< min.count() << std::endl;

            if (err & std::ios_base::failbit) goto exit;
            time_t t;
#if BOOST_CHRONO_INTERNAL_TIMEGM
            t = detail::internal_timegm(&tm);
#else
            t = timegm(&tm);
#endif
            tp = time_point_cast<Duration>(
                system_clock::from_time_t(t) - min + round<microseconds> (duration<double> (sec))
                );
          }
          else
          {
            const CharT z[2] =
            { '%', 'z' };
            const CharT* fz = std::search(pb, pe, z, z + 2);
#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
            const detail::time_get<CharT>& dtg(tg);
            dtg.get(is, 0, is, err, &tm, pb, fz);
#else
            tg.get(is, 0, is, err, &tm, pb, fz);
#endif
            minutes minu(0);
            if (fz != pe)
            {
              if (err != std::ios_base::goodbit)
              {
                err |= std::ios_base::failbit;
                goto exit;
              }
              It i(is);
              It eof;
              minu = detail::extract_z(i, eof, err, ct);
              if (err & std::ios_base::failbit) goto exit;
              if (fz + 2 != pe)
              {
                if (err != std::ios_base::goodbit)
                {
                  err |= std::ios_base::failbit;
                  goto exit;
                }
#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
                const detail::time_get<CharT>& dtg(tg);
                dtg.get(is, 0, is, err, &tm, fz + 2, pe);
#else
                tg.get(is, 0, is, err, &tm, fz + 2, pe);
#endif
                if (err & std::ios_base::failbit) goto exit;
              }
            }
            tm.tm_isdst = -1;
            time_t t;
            if (tz == timezone::utc || fz != pe)
            {
#if BOOST_CHRONO_INTERNAL_TIMEGM
              t = detail::internal_timegm(&tm);
#else
              t = timegm(&tm);
#endif
            }
            else
            {
              t = mktime(&tm);
            }
            tp = time_point_cast<Duration>(
                system_clock::from_time_t(t) - minu
                );
          }
        }
#ifndef BOOST_NO_EXCEPTIONS
        catch (...) // BOOST_NO_EXCEPTIONS protected
        {
          err |= std::ios_base::badbit | std::ios_base::failbit;
        }
#endif
        exit: is.setstate(err);
      }
      return is;
    }
#endif
#endif //UTC
  } // chrono

}

#endif  // header
