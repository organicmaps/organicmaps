#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "routing/routes_builder/routes_builder.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace routing
{
namespace routes_builder
{
void BuildRoutes(std::string const & routesPath, std::string const & dumpPath, uint64_t startFrom,
                 uint64_t threadsNumber, uint32_t timeoutPerRouteSeconds, std::string const & vehicleType, bool verbose,
                 uint32_t launchesNumber);

void BuildRoutesWithApi(std::unique_ptr<routing_quality::api::RoutingApi> routingApi, std::string const & routesPath,
                        std::string const & dumpPath, int64_t startFrom);

std::unique_ptr<routing_quality::api::RoutingApi> CreateRoutingApi(std::string const & name, std::string const & token);
}  // namespace routes_builder
}  // namespace routing
