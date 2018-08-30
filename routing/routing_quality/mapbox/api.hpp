#pragma once

#include "routing/routing_quality/mapbox/types.hpp"
#include "routing/routing_quality/utils.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace routing_quality
{
namespace mapbox
{
class Api
{
public:
  explicit Api(std::string const & accessToken) : m_accessToken(accessToken) {}

  DirectionsResponse MakeDirectionsRequest(RouteParams const & params) const;
  void DrawRoutes(Geometry const & mapboxRoute, Geometry const & mapsmeRoute, std::string const & snapshotPath) const;

  static std::vector<std::string> GenerateWaypointsBasedOn(DirectionsResponse response);

  template <typename Container>
  static void Sieve(Container & cont, size_t maxRemainingNumber)
  {
    CHECK_GREATER(maxRemainingNumber, 0, ());
    auto const size = cont.size();
    if (size <= maxRemainingNumber)
      return;

    Container res;
    res.reserve(maxRemainingNumber);
    auto const step = size / maxRemainingNumber;
    for (size_t i = 0; i < size; i += step)
      res.emplace_back(cont[i]);

    cont = std::move(res);
  }

private:
  std::string GetDirectionsURL(RouteParams const & params) const;
  std::string GetRoutesRepresentationURL(Geometry const & mapboxRoute, Geometry const & mapsmeRoute) const;

  std::string m_accessToken;
};
}  // namespace mapbox
}  // namespace routing_quality
