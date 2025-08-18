#pragma once

#include "routing/routes_builder/routes_builder.hpp"

#include <string>
#include <utility>
#include <vector>

namespace routing_quality::routing_quality_tool
{
using RouteResult = routing::routes_builder::RoutesBuilder::Result;
void RunBenchmarkStat(std::vector<std::pair<RouteResult, std::string>> const & mapsmeResults,
                      std::string const & dirForResults);

void RunBenchmarkComparison(std::vector<std::pair<RouteResult, std::string>> && mapsmeResults,
                            std::vector<std::pair<RouteResult, std::string>> && mapsmeOldResults,
                            std::string const & dirForResults);
}  // namespace routing_quality::routing_quality_tool
