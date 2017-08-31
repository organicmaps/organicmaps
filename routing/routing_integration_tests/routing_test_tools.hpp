#pragma once

#include "routing/index_router.hpp"

#include "storage/country_info_getter.hpp"

#include "map/feature_vec_model.hpp"

#include "platform/local_country_file.hpp"

#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

/*
 * These tests are developed to simplify routing integration tests writing.
 * You can use the interface bellow however you want but there are some hints.
 * 1. Most likely you want to use GetCarComponents() or GetPedestrianComponents() without parameter
 *    to get a reference to IRouterComponents.
 *    It loads all the maps from directories Platform::WritableDir()
 *    and Platform::ResourcesDir() only once and then reuse it.
 *    Use GetCarComponents() or GetPedestrianComponents() with vector of maps parameter
 *    only if you want to test something on a special map set.
 * 2. Loading maps and calculating routes is a time consumption process.
 *    Do this only if you really need it.
 * 3. If you want to check that a turn is absent - use TestTurnCount.
 * 4. The easiest way to gather all the information for writing an integration test is
 *    - to put a break point in IRouter::CalculateRoute() method;
 *    - to make a route with MapWithMe desktop application;
 *    - to get all necessary parameters and result of the route calculation;
 *    - to place them into the test you're writing.
 * 5. The recommended way for naming tests for a route from one place to another one is
 *    <Country><City><Street1><House1><Street2><House2><Test time. TurnTest or RouteTest for the
 * time being>
 * 6. It's a good idea to use short routes for testing turns. The thing is geometry of long routes
 *    could be changes for one dataset to another one. The shorter route the less chance it'll be changed.
 */
using namespace routing;
using namespace turns;
using platform::LocalCountryFile;

typedef pair<shared_ptr<Route>, IRouter::ResultCode> TRouteResult;

namespace integration
{
shared_ptr<model::FeaturesFetcher> CreateFeaturesFetcher(vector<LocalCountryFile> const & localFiles);

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter();

class IRouterComponents
{
public:
  IRouterComponents(vector<LocalCountryFile> const & localFiles)
    : m_featuresFetcher(CreateFeaturesFetcher(localFiles)), m_infoGetter(CreateCountryInfoGetter())
  {
  }

  virtual ~IRouterComponents() = default;

  virtual IRouter * GetRouter() const = 0;

  storage::CountryInfoGetter const & GetCountryInfoGetter() const noexcept { return *m_infoGetter; }

protected:
  shared_ptr<model::FeaturesFetcher> m_featuresFetcher;
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;
};

void TestOnlineCrosses(ms::LatLon const & startPoint, ms::LatLon const & finalPoint,
                       vector<string> const & expected, IRouterComponents & routerComponents);
void TestOnlineFetcher(ms::LatLon const & startPoint, ms::LatLon const & finalPoint,
                       vector<string> const & expected, IRouterComponents & routerComponents);

// Gets car router components
IRouterComponents & GetCarComponents();

// Gets pedestrian router components
IRouterComponents & GetPedestrianComponents();

// Gets bicycle router components.
IRouterComponents & GetBicycleComponents();

TRouteResult CalculateRoute(IRouterComponents const & routerComponents,
                            m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint);

void TestTurnCount(Route const & route, uint32_t expectedTurnCount);

/// Testing route length.
/// It is used for checking if routes have expected(sample) length.
/// A created route will pass the test iff
/// expectedRouteMeters - expectedRouteMeters * relativeError  <= route->GetDistance()
/// && expectedRouteMeters + expectedRouteMeters * relativeError >= route->GetDistance()
void TestRouteLength(Route const & route, double expectedRouteMeters, double relativeError = 0.01);
void TestRouteTime(Route const & route, double expectedRouteSeconds, double relativeError = 0.01);

void CalculateRouteAndTestRouteLength(IRouterComponents const & routerComponents,
                                      m2::PointD const & startPoint,
                                      m2::PointD const & startDirection,
                                      m2::PointD const & finalPoint, double expectedRouteMeters,
                                      double relativeError = 0.07);

class TestTurn
{
  friend TestTurn GetNthTurn(Route const & route, uint32_t turnNumber);

  m2::PointD const m_point;
  CarDirection const m_direction;
  uint32_t const m_roundAboutExitNum;
  bool const m_isValid;

  TestTurn()
    : m_point({0.0, 0.0})
    , m_direction(CarDirection::None)
    , m_roundAboutExitNum(0)
    , m_isValid(false)
  {
  }
  TestTurn(m2::PointD const & pnt, CarDirection direction, uint32_t roundAboutExitNum)
    : m_point(pnt), m_direction(direction), m_roundAboutExitNum(roundAboutExitNum), m_isValid(true)
  {
  }

public:
  const TestTurn & TestValid() const;
  const TestTurn & TestNotValid() const;
  const TestTurn & TestPoint(m2::PointD const & expectedPoint, double inaccuracyMeters = 3.) const;
  const TestTurn & TestDirection(CarDirection expectedDirection) const;
  const TestTurn & TestOneOfDirections(set<CarDirection> const & expectedDirections) const;
  const TestTurn & TestRoundAboutExitNum(uint32_t expectedRoundAboutExitNum) const;
};

/// Extracting appropriate TestTurn if any. If not TestTurn::isValid() returns false.
/// inaccuracy is set in meters.
TestTurn GetNthTurn(Route const & route, uint32_t turnNumber);

void TestCurrentStreetName(routing::Route const & route, string const & expectedStreetName);

void TestNextStreetName(routing::Route const & route, string const & expectedStreetName);
}  // namespace integration
