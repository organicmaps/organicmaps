#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "routing/routing_quality/api/mapbox/types.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace routing_quality
{
namespace api
{
namespace mapbox
{
class MapboxApi : public RoutingApi
{
public:
  explicit MapboxApi(std::string const & token);

  // According to:
  // https://docs.mapbox.com/api/navigation/#directions-api-restrictions-and-limits
  static uint32_t constexpr kMaxRPS = 5;
  static std::string const kApiName;

  // RoutingApi overrides:
  // @{
  Response CalculateRoute(Params const & params, int32_t /* startTimeZoneUTC */) const override;
  // @}

private:
  MapboxResponse MakeRequest(Params const & params) const;
  std::string GetDirectionsURL(Params const & params) const;
};
}  // namespace mapbox
}  // namespace api
}  // namespace routing_quality
