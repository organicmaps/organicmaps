#pragma once

#include "metrics/eye_info.hpp"

namespace place_page
{
class Info;
}

class FeatureType;

namespace utils
{
eye::MapObject MakeEyeMapObject(place_page::Info const & info);
eye::MapObject MakeEyeMapObject(FeatureType & ft);
}  // namespace utils
