#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "routing/routing_quality/api/google/types.hpp"

#include <cstdint>
#include <string>

namespace routing_quality
{
namespace api
{
namespace google
{
class GoogleApi : public RoutingApi
{
public:
  explicit GoogleApi(std::string const & token);

  // According to:
  // https://developers.google.com/maps/faq#usage_apis
  static uint32_t constexpr kMaxRPS = 50;
  static std::string const kApiName;

  // RoutingApi overrides:
  // @{
  Response CalculateRoute(Params const & params, int32_t startTimeZoneUTC) const override;
  // @}

private:
  GoogleResponse MakeRequest(Params const & params, int32_t startTimeZoneUTC) const;
  std::string GetDirectionsURL(Params const & params, int32_t startTimeZoneUTC) const;
};
}  // namespace google
}  // namespace api
}  // namespace routing_quality
