#pragma once

#include "coding/string_utf8_multilang.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

namespace kml
{
auto constexpr kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto constexpr kDefaultTrackWidth = 5.0;
auto constexpr kDefaultTrackColor = 0x006ec7ff;

template <typename Channel>
uint32_t ToRGBA(Channel red, Channel green, Channel blue, Channel alpha)
{
  return static_cast<uint8_t>(red) << 24 | static_cast<uint8_t>(green) << 16 |
         static_cast<uint8_t>(blue) << 8 | static_cast<uint8_t>(alpha);
}

std::string PointToString(m2::PointD const & org);

std::string PointToString(geometry::PointWithAltitude const & pt);

}  // namespace kml
