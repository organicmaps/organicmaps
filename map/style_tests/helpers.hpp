#pragma once

#include "indexer/map_style.hpp"

#include "std/function.hpp"

namespace styles
{
void RunForEveryMapStyle(function<void(MapStyle)> const & fn);
} // namespace styles
