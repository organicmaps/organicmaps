#include "../testing/testing.hpp"

#include "osrm_test_tools.hpp"

#include "../indexer/mercator.hpp"

using namespace routing;

namespace
{
  UNIT_TEST(RussiaMoscowLenigradskiy39GerPanfilovtsev22RouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.53758809983519, 67.536162466434234},
                                                  {0., 0.}, {37.40993977728661, 67.644784047393685}, 14296.);
  }

  UNIT_TEST(RussiaMoscowGerPanfilovtsev22SolodchaPravdiRouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.409929478750627, 67.644798619710073},
                                                  {0., 0.}, {39.836562407458047, 65.774372510437971}, 239426.);
  }

  UNIT_TEST(UKRugbyStIvesRouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {-1.2653036222483705, 61.691304855049886},
                                                  {0., 0.}, {-5.4799407508360218, 58.242809563579847}, 455902.);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39ItalySienaCenterRouteTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.537596024929826, 67.536160359657288},
                                                  {0., 0.}, {11.327927635052676, 48.166256203616726}, 2870710.);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39EnglandLondonCenterRouteTest)
  {
    //@todo put down a correct route length when router is fixed
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.537572384446207, 67.536189683408367},
                                                  {0., 0.}, {-0.084976483156808751, 60.298304898120428}, 1000.);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39RepublicOfSouthAfricaCapeTownCenterRouteTest)
  {
    //@todo put down a correct route length when router is fixed
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {37.537543510152318, 67.536217686389165},
                                                  {0., 0.}, {18.542688617866236, -36.095015335418523}, 1000.);
  }

  UNIT_TEST(ArbatBaliCrimeanForwardCrossMwmTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents,  {MercatorBounds::LonToX(34.804955), MercatorBounds::LatToY(46.152324)},
                                                  {0., 0.}, {MercatorBounds::LonToX(35.369712), MercatorBounds::LatToY(45.356971)}, 105000.);
  }

  UNIT_TEST(ArbatBaliCrimeanBackwardCrossTest)
  {
    shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
    integration::CalculateRouteAndTestRouteLength(routerComponents, {MercatorBounds::LonToX(35.369712), MercatorBounds::LatToY(45.356971)},
                                                  {0., 0.}, {MercatorBounds::LonToX(34.804955), MercatorBounds::LatToY(46.152324)}, 105000.);
  }
}
