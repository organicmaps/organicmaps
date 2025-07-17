#pragma once

#include "base/exception.hpp"

namespace routing
{
DECLARE_EXCEPTION(RoutingException, RootException);
DECLARE_EXCEPTION(CorruptedDataException, RoutingException);
DECLARE_EXCEPTION(MwmIsNotAliveException, RoutingException);
}  // namespace routing
