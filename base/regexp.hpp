#pragma once

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_token_iterator.hpp>


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

  template <class FnT> void ForEachMatched(string const & str, RegExpT const & regexp, FnT fn)
  {
    typedef boost::xpressive::sregex_token_iterator IterT;

    IterT i(str.begin(), str.end(), regexp);
    IterT end;
    for (; i != end; ++i)
      fn(*i);
  }
}
