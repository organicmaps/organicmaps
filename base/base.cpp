#include "base/base.hpp"
#include "base/assert.hpp"
#include "base/exception.hpp"

#include "std/target_os.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>

namespace my
{
  void OnAssertFailedDefault(SrcPoint const & srcPoint, std::string const & msg)
  {
    std::cerr << "ASSERT FAILED" << std::endl
              << srcPoint.FileName() << ":" << srcPoint.Line() << std::endl
              << msg << std::endl;

#ifdef DEBUG
    assert(false);
#else
    std::abort();
#endif
  }

  AssertFailedFn OnAssertFailed = &OnAssertFailedDefault;

  AssertFailedFn SetAssertFunction(AssertFailedFn fn)
  {
    std::swap(OnAssertFailed, fn);
    return fn;
  }
}
