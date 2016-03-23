#pragma once

#include "std/ctime.hpp"

namespace my
{
/// A cross-platform replacenemt of gmtime_r
std::tm GmTime(time_t const time);
}  // namespace my
