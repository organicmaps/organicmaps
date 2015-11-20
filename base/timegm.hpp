#pragma once

#include "std/ctime.hpp"

// For some reasons android doesn't have timegm function. The following
// work around is provided.
namespace base
{
time_t TimeGM(std::tm const & tm);
}
