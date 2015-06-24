#pragma once
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <iomanip>
using std::fixed;
using std::hex;
using std::left;
using std::setfill;
using std::setprecision;
using std::setw;

// TODO: Should we force clang/libc++ here?
#ifndef OMIM_OS_LINUX
using std::get_time;
using std::put_time;

#else
#include <cassert>

namespace detail
{
  template <class _CharT> struct get_time_manip
  {
    tm* __tm_;
    const _CharT* __fmt_;

    get_time_manip(tm* __tm, const _CharT* __fmt)
        : __tm_(__tm), __fmt_(__fmt) {}
  };

  template <class _CharT, class _Traits>
  class stream_buf_impl : public std::basic_streambuf<_CharT, _Traits>
  {
    typedef std::basic_streambuf<_CharT, _Traits> base_t;
  public:
    bool parse(const get_time_manip<_CharT>& __x)
    {
      // Workaround works only for a stream buffer under null-terminated string.
      assert(*base_t::egptr() == 0);

      char * res = ::strptime(base_t::gptr(), __x.__fmt_, __x.__tm_);
      if (res == 0)
        return false;
      else
      {
        base_t::setg(base_t::eback(), res, base_t::egptr());
        return true;
      }
    }
  };

  template <class _CharT, class _Traits>
  std::basic_istream<_CharT, _Traits>&
  operator>>(std::basic_istream<_CharT, _Traits>& __is, const get_time_manip<_CharT>& __x)
  {
    if (!reinterpret_cast<stream_buf_impl<_CharT, _Traits>*>(__is.rdbuf())->parse(__x))
      __is.setstate(std::ios_base::failbit);
    return __is;
  }
}

template <class _CharT>
detail::get_time_manip<_CharT> get_time(tm* __tm, const _CharT* __fmt)
{
  return detail::get_time_manip<_CharT>(__tm, __fmt);
}

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
