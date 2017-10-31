#pragma once

#include "base/exception.hpp"

namespace routing
{
DECLARE_EXCEPTION(CorruptedDataException, RootException);
DECLARE_EXCEPTION(RoutingException, RootException);
DECLARE_EXCEPTION(MwmIsNotAliveException, RootException);
}  // namespace routing
