#pragma once

#include "base/cancellable.hpp"
#include "base/exception.hpp"

namespace search
{
// This exception can be thrown from the deep darkness of search and
// geometry retrieval for fast cancellation of time-consuming tasks.
DECLARE_EXCEPTION(CancelException, RootException);

inline void BailIfCancelled(base::Cancellable const & cancellable)
{
  if (cancellable.IsCancelled())
    MYTHROW(CancelException, ("Cancelled"));
}
}  // namespace search
