#pragma once

#include "search/reverse_geocoder.hpp"

#include "metrics/eye_info.hpp"

#include "geometry/point2d.hpp"

namespace place_page
{
class Info;
}

class DataSource;
class FeatureType;

namespace utils
{
eye::MapObject MakeEyeMapObject(place_page::Info const & info);
eye::MapObject MakeEyeMapObject(FeatureType & ft);

search::ReverseGeocoder::Address GetAddressAtPoint(DataSource const & dataSource,
                                                   m2::PointD const & pt);
}  // namespace utils
