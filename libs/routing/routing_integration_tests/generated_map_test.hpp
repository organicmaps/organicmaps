#pragma once

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/vehicle_mask.hpp"

#include "generator/generator_tests_support/test_generator.hpp"

#include <memory>
#include <set>
#include <string>

namespace routing
{
class Route;
}  // namespace routing

namespace integration
{
// Generates an mwm with features, routing and cross-mwm sections from an OSM XML file and exposes
// router components for it, so a route can be calculated on synthetic OSM data without any
// downloaded maps.
class GeneratedMapTest
{
public:
  // |mwmName| should be the real country name that CountryInfoGetter resolves for the test
  // coordinates: the router locates the mwm by point -> country id.
  GeneratedMapTest(std::string const & osmFile, std::string const & mwmName, routing::VehicleType vehicleType);

  std::string const & GetMwmPath() const { return m_mwmPath; }
  IRouterComponents const & GetComponents() const { return *m_components; }

  // Returns the OSM way ids of the real (non-fake) segments |route| passes through, mapping each
  // route feature back to its source OSM id via the generated osm2ft section.
  std::set<uint64_t> GetUsedOsmWays(routing::Route const & route);

private:
  generator::tests_support::TestRawGenerator m_generator;
  std::string m_mwmName;
  std::string m_mwmPath;
  std::unique_ptr<VehicleRouterComponents> m_components;
};
}  // namespace integration
