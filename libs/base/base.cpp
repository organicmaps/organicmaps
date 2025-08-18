#include "base/base.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/src_point.hpp"

#include <iostream>

namespace base
{
bool OnAssertFailedDefault(SrcPoint const & srcPoint, std::string const & msg)
{
  auto & logger = LogHelper::Instance();

  std::cerr << '(' << logger.GetThreadID() << ") ASSERT FAILED" << '\n'
            << srcPoint.FileName() << ':' << srcPoint.Line() << '\n'
            << msg << std::endl
            << std::flush;
  return true;
}

AssertFailedFn OnAssertFailed = &OnAssertFailedDefault;

AssertFailedFn SetAssertFunction(AssertFailedFn fn)
{
  std::swap(OnAssertFailed, fn);
  return fn;
}
}  // namespace base
