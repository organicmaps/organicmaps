#include "testing/testing.hpp"

#include "routing/routing_quality/utils.hpp"
#include "routing/routing_quality/waypoints.hpp"

#include <utility>
#include <vector>

using namespace std;
using namespace routing_quality;

namespace
{
UNIT_TEST(RoutingQuality_CompareSmoke)
{
  // From office to Aseeva 6.
  RouteParams params;
  params.m_waypoints = {{55.79723, 37.53777},
                        {55.80634, 37.52886}};

  ReferenceRoutes candidates;
  ReferenceRoute first;
  first.m_waypoints = {{55.79676, 37.54138},
                       {55.79914, 37.53582},
                       {55.80353, 37.52478},
                       {55.80556, 37.52770}};
  first.m_factor = 1.0;
  candidates.emplace_back(move(first));

  TEST_EQUAL(CheckWaypoints(move(params), move(candidates)), 1.0, ());
}

UNIT_TEST(RoutingQuality_Zelegonrad2Domodedovo)
{
  // From Zelenograd to Domodedovo.
  RouteParams params;
  params.m_waypoints = {{55.98301, 37.21141},
                        {55.42081, 37.89361}};

  ReferenceRoutes candidates;

  // Through M-11 and MCA.
  ReferenceRoute first;
  first.m_waypoints = {{55.99751, 37.23804},
                       {56.00719, 37.28533},
                       {55.88759, 37.48068},
                       {55.83513, 37.39569},
                       {55.57103, 37.69203}};
  first.m_factor = 1.0;
  candidates.emplace_back(move(first));

  // Through M-10 and MCA.
  ReferenceRoute second;
  second.m_waypoints = {{55.99775, 37.24941},
                        {55.88627, 37.43915},
                        {55.86882, 37.40784},
                        {55.58645, 37.71672},
                        {55.57855, 37.75468}};
  second.m_factor = 1.0;
  candidates.emplace_back(move(second));

  // Through M-10 and Moscow center.
  ReferenceRoute third;
  third.m_waypoints = {{55.98974, 37.26966},
                       {55.87625, 37.45129},
                       {55.78288, 37.57118},
                       {55.76092, 37.60087},
                       {55.75662, 37.59861},
                       {55.74976, 37.60654},
                       {55.73173, 37.62060},
                       {55.71785, 37.62237},
                       {55.65615, 37.64623},
                       {55.57855, 37.75468}};
  third.m_factor = 1.0;
  candidates.emplace_back(move(third));

  TEST_EQUAL(CheckWaypoints(move(params), move(candidates)), 1.0, ());
}

UNIT_TEST(RoutingQuality_Sokol2Mayakovskaya)
{
  // From Sokol to Mayakovskaya through Leningradsky Avenue but not through its alternate.
  RouteParams params;
  params.m_waypoints = {{55.80432, 37.51603},
                        {55.77019, 37.59558}};

  ReferenceRoutes candidates;
  ReferenceRoute first;
  // All points lie on the alternate so the result should be 0.
  first.m_waypoints = {{55.79599, 37.54114},
                       {55.78142, 37.57364},
                       {55.77863, 37.57989}};
  first.m_factor = 1.0;
  candidates.emplace_back(move(first));

  TEST_EQUAL(CheckWaypoints(move(params), move(candidates)), 0.0, ());
}
}  // namespace
