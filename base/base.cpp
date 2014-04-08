#include "base.hpp"
#include "assert.hpp"
#include "exception.hpp"

#include "../std/target_os.hpp"
#include "../std/iostream.hpp"

#include <cassert>

#ifdef OMIM_OS_TIZEN
  #include <FBaseSys.h>
#endif

namespace my
{
  void OnAssertFailedDefault(SrcPoint const & srcPoint, string const & msg)
  {
#ifdef OMIM_OS_TIZEN
    AppLog("ASSERT FAILED%s:%d:%s", srcPoint.FileName(), srcPoint.Line(), msg.c_str());
    AppAssert(false);

#else
    std::cerr << "ASSERT FAILED\n" << srcPoint.FileName() << ":" << srcPoint.Line() << "\n"
               << msg << endl;

#ifdef DEBUG
    assert(false);
#else
    MYTHROW(RootException, (msg));
#endif

#endif
  }

  AssertFailedFn OnAssertFailed = &OnAssertFailedDefault;

  AssertFailedFn SetAssertFunction(AssertFailedFn fn)
  {
    std::swap(OnAssertFailed, fn);
    return fn;
  }
}
