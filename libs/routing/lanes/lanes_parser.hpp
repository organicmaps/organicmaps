#pragma once

#include "routing/lanes/lane_info.hpp"

#include <vector>

namespace routing::turns::lanes
{
/**
 * Parse lane information which comes from lanesString
 * @param lanesString lane information. Example through|through|through|through;right
 * @return LanesInfo. @see LanesInfo
 * @note if lanesString is empty, returns empty LanesInfo.
 */
LanesInfo ParseLanes(std::string_view lanesString);
}  // namespace routing::turns::lanes
