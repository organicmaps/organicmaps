#pragma once

#include "ugc/types.hpp"

namespace ugc
{
UGC MakeTestUGC1(Time now = Clock::now());
UGC MakeTestUGC2(Time now = Clock::now());
UGCUpdate MakeTestUGCUpdate(Time now = Clock::now());
}  // namespace ugc
