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
std::vector<RoutesBuilder::Result> BuildRoutes(std::string const & routesPath,
                                               std::string const & dumpPath,
                                               int64_t startFrom,
                                               uint64_t threadsNumber);

void BuildRoutesWithApi(std::unique_ptr<routing_quality::api::RoutingApi> routingApi,
                        std::string const & routesPath,
                        std::string const & dumpPath,
                        int64_t startFrom);

std::unique_ptr<routing_quality::api::RoutingApi> CreateRoutingApi(std::string const & name,
                                                                   std::string const & token);
}  // namespace routes_builder
}  // routing
