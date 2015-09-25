#include "base/base.hpp"
#include "base/assert.hpp"
#include "base/exception.hpp"

#include "std/target_os.hpp"
#include "std/iostream.hpp"

#include <cassert>
#include <cstdlib>


namespace my
{
  void OnAssertFailedDefault(SrcPoint const & srcPoint, string const & msg)
  {
    std::cerr << "ASSERT FAILED" << endl
              << srcPoint.FileName() << ":" << srcPoint.Line() << endl
              << msg << endl;

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
