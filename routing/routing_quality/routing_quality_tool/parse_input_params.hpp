#pragma once

#include "routing/routing_quality/utils.hpp"

#include "coding/file_reader.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <iterator>
#include <string>
#include <vector>

namespace routing_quality
{
template <typename Iter, typename Container>
void SerializeLatLon(Iter begin, Iter end, Container & c)
{
  c.reserve(std::distance(begin, end));
  for (auto it = begin; it != end; ++it)
  {
    auto const & str = *it;
    auto const coords = strings::Tokenize(str, ",");
    CHECK_EQUAL(coords.size(), 2, ("Incorrect string", str));

    double lat = 0.0;
    double lon = 0.0;
    CHECK(strings::to_double(coords[0], lat), ("Incorrect string", coords[0]));
    CHECK(strings::to_double(coords[1], lon), ("Incorrect string", coords[1]));
    CHECK(MercatorBounds::ValidLat(lat), ("Incorrect lat", lat));
    CHECK(MercatorBounds::ValidLon(lon), ("Incorrect lon", lon));

    c.emplace_back(lat, lon);
  }
}

RouteParams SerializeRouteParamsFromFile(std::string const & routeParamsFilePath)
{
  FileReader r(routeParamsFilePath);
  std::string data;
  r.ReadAsString(data);

  auto const tokenized = strings::Tokenize(data, ";");
  auto const size = tokenized.size();
  CHECK_GREATER_OR_EQUAL(size, 3, ("Incorrect string", data));

  RouteParams params;
  SerializeLatLon(tokenized.begin(), tokenized.end() - 1, params.m_waypoints);

  int type = 0;
  CHECK(strings::to_int(tokenized[size - 1], type), ("Incorrect string", tokenized[size - 1]));
  CHECK_LESS(type, static_cast<int>(routing::VehicleType::Count), ("Incorrect vehicle type", type));
  params.m_type = static_cast<routing::VehicleType>(type);
  return params;
}

RouteParams SerializeRouteParamsFromString(std::string const & waypoints, int vehicleType)
{
  auto const tokenized = strings::Tokenize(waypoints, ";");
  auto const size = tokenized.size();
  CHECK_GREATER_OR_EQUAL(size, 2, ("Incorrect string", waypoints));

  RouteParams params;
  SerializeLatLon(tokenized.begin(), tokenized.end(), params.m_waypoints);

  CHECK_LESS(vehicleType, static_cast<int>(routing::VehicleType::Count), ("Incorrect vehicle type", vehicleType));
  CHECK_GREATER_OR_EQUAL(vehicleType, 0, ("Incorrect vehicle type", vehicleType));
  params.m_type = static_cast<routing::VehicleType>(vehicleType);
  return params;
}
}  // namespace routinq_quality
