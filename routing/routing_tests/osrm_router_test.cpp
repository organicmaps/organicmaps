#include "../../testing/testing.hpp"
#include "../osrm_router.hpp"
#include "../../indexer/mercator.hpp"

using namespace routing;

namespace
{

void Callback(Route const & r)
{

}

}

UNIT_TEST(OsrmRouter_Test)
{
  OsrmRouter router;
  router.SetFinalPoint(m2::PointD(MercatorBounds::LonToX(27.54334), MercatorBounds::LatToY(53.899338)));  // офис Немига
  router.CalculateRoute(m2::PointD(MercatorBounds::LonToX(27.6704144), MercatorBounds::LatToY(53.9456243)), Callback);  // Городецкая
}
