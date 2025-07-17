#pragma once

#include <string>

#include "base/macros.hpp"
#include "base/thread_checker.hpp"

namespace base
{
// This class is a simple RAII wrapper around gperftools profiler.  It
// is NOT thread-safe, moreover, before using it you must be sure that
// there are no interfering instantiations of PProf during the
// execution of the code you are interested in.
class PProf final
{
public:
  // Starts profiling and writes profile info into |path|, discarding
  // any existing profiling data in that file.
  PProf(std::string const & path);

  // Stops profiling.
  ~PProf();

private:
  ThreadChecker m_checker;

  DISALLOW_COPY_AND_MOVE(PProf);
};
}  // namespace base
