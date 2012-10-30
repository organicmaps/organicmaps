#pragma once

#include <boost/xpressive/xpressive.hpp>

namespace regexp
{
  typedef boost::xpressive::sregex RegExpT;

  inline void Create(string const & regexp, RegExpT & out)
  {
    out = RegExpT::compile(regexp);
  }

  inline bool IsExist(string const & str, RegExpT const & regexp)
  {
    return boost::xpressive::regex_search(str, regexp);
  }
}
