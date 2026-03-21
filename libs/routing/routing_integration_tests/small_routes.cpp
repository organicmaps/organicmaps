#include "testing/testing.hpp"

#include "routing/vehicle_mask.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <tuple>
#include <vector>

namespace
{
// This is set of small routes was received from users' crashes.
// It crashed into astar_algorithm.hpp in CHECK() about A* invariant
// for bidirectional algo.
// These tests should just passed without any crash.
UNIT_TEST(SmallRoutes_JustNoError)
{
  // Do not touch the coords here! It's must be written in this format.
  // In other case, some tests become not crashable.
  std::vector<std::tuple<ms::LatLon, ms::LatLon, routing::VehicleType>> routes = {
      {{12.087755, -68.898342}, {12.086652, -68.899154}, routing::VehicleType::Car},         // 1
      {{-20.104568, 57.580254}, {-20.106874, 57.580414}, routing::VehicleType::Car},         // 2
      {{-0.756955, -90.315459}, {-0.757317, -90.314971}, routing::VehicleType::Pedestrian},  // 3
      {{-13.527445, -71.983072}, {-13.527530, -71.983102}, routing::VehicleType::Car},       // 4
      {{-22.205002, 166.472641}, {-22.205165, 166.473012}, routing::VehicleType::Car},       // 5
      {{-33.640811, 115.023226}, {-33.640719, 115.022691}, routing::VehicleType::Car},       // 6
      {{-37.731474, 176.132243}, {-37.731238, 176.132445}, routing::VehicleType::Car},       // 7
      {{-37.950809, 176.993992}, {-37.951158, 176.994054}, routing::VehicleType::Car},       // 8
      {{-38.122103, 176.307336}, {-38.122196, 176.307424}, routing::VehicleType::Car},       // 9
      {{-44.384889, 171.245860}, {-44.384856, 171.246203}, routing::VehicleType::Car},       // 10
      {{-8.352453, 116.039122}, {-8.352394, 116.038901}, routing::VehicleType::Pedestrian},  // 11
      {{-8.690502, 115.432713}, {-8.690827, 115.433297}, routing::VehicleType::Pedestrian},  // 12
      {{-8.840441, 115.180369}, {-8.840584, 115.180666}, routing::VehicleType::Car},         // 13
      {{10.766800, 106.691260}, {10.766686, 106.691217}, routing::VehicleType::Car},         // 14
      {{12.089446, -68.863145}, {12.089641, -68.863023}, routing::VehicleType::Car},         // 15
      {{12.168143, -68.285843}, {12.168203, -68.285999}, routing::VehicleType::Car},         // 16
      {{12.490231, -69.966714}, {12.489738, -69.966762}, routing::VehicleType::Car},         // 17
      {{12.529459, -69.989523}, {12.529752, -69.989532}, routing::VehicleType::Car},         // 18
      {{12.545208, -70.052132}, {12.545086, -70.051987}, routing::VehicleType::Car},         // 19
      {{14.623702, 121.015163}, {14.623624, 121.015058}, routing::VehicleType::Car},         // 20
      {{27.614768, -82.735309}, {27.615303, -82.735014}, routing::VehicleType::Car},         // 21
      {{30.647048, 104.071743}, {30.647142, 104.071331}, routing::VehicleType::Pedestrian},  // 22
      {{31.308356, 120.629667}, {31.308911, 120.629565}, routing::VehicleType::Pedestrian},  // 23
      {{31.624363, -7.985198}, {31.624537, -7.985214}, routing::VehicleType::Pedestrian},    // 24
      {{31.624519, -7.985267}, {31.624537, -7.985214}, routing::VehicleType::Pedestrian},    // 25
      {{31.656752, 34.555609}, {31.656685, 34.555488}, routing::VehicleType::Car},           // 26
      {{34.907010, 33.620845}, {34.906048, 33.620263}, routing::VehicleType::Car},           // 27
      {{34.921187, 32.625802}, {34.921086, 32.625805}, routing::VehicleType::Car},           // 28
      {{34.979258, 33.937110}, {34.979098, 33.937177}, routing::VehicleType::Car},           // 29
      {{35.018181, 34.046807}, {35.018514, 34.046997}, routing::VehicleType::Car},           // 30
      {{35.514115, 23.912056}, {35.514230, 23.912168}, routing::VehicleType::Car},           // 31
      {{35.581194, 45.424180}, {35.581028, 45.424491}, routing::VehicleType::Pedestrian},    // 32
      {{36.230656, 28.134475}, {36.230385, 28.134924}, routing::VehicleType::Car},           // 33
      {{36.358085, 25.444976}, {36.358666, 25.445164}, routing::VehicleType::Pedestrian},    // 34
      {{36.694829, 31.598065}, {36.694798, 31.597773}, routing::VehicleType::Car},           // 35
      {{37.090061, -8.413102}, {37.089739, -8.412828}, routing::VehicleType::Pedestrian},    // 36
      {{40.166637, 44.442915}, {40.166747, 44.442915}, routing::VehicleType::Car},           // 37
      {{40.166670, 44.442903}, {40.166744, 44.442910}, routing::VehicleType::Car},           // 38
      {{40.171230, 44.539676}, {40.171196, 44.539501}, routing::VehicleType::Car},           // 39
      {{40.546780, 14.243017}, {40.546760, 14.242680}, routing::VehicleType::Pedestrian},    // 40
      {{41.384478, 2.161366}, {41.384550, 2.161269}, routing::VehicleType::Car},             // 41
      {{42.966040, -9.039394}, {42.965825, -9.038590}, routing::VehicleType::Pedestrian},    // 42
      {{43.705792, -79.422995}, {43.705667, -79.422915}, routing::VehicleType::Car},         // 43
      {{43.856909, 18.384329}, {43.857161, 18.384610}, routing::VehicleType::Car},           // 44
      {{43.973927, 3.134214}, {43.973684, 3.134444}, routing::VehicleType::Car},             // 45
      {{44.413185, 12.206472}, {44.413859, 12.206783}, routing::VehicleType::Car},           // 46
      {{44.420630, 26.161473}, {44.420488, 26.161505}, routing::VehicleType::Car},           // 47
      {{44.599012, 33.460076}, {44.598730, 33.460236}, routing::VehicleType::Car},           // 48
      {{45.249033, 19.799025}, {45.249157, 19.798848}, routing::VehicleType::Car},           // 49
      {{46.144412, -62.467054}, {46.144296, -62.466491}, routing::VehicleType::Car},         // 50
      {{46.171920, 21.350128}, {46.172083, 21.350218}, routing::VehicleType::Car},           // 51
      {{47.033895, 28.843783}, {47.033992, 28.843885}, routing::VehicleType::Car},           // 52
      {{47.169674, 11.385885}, {47.169627, 11.385719}, routing::VehicleType::Car},           // 53
      {{47.921071, 106.880479}, {47.920870, 106.880295}, routing::VehicleType::Car},         // 54
      {{48.390422, 24.501536}, {48.390864, 24.499478}, routing::VehicleType::Car},           // 55
      {{48.631019, 25.740152}, {48.631189, 25.739863}, routing::VehicleType::Car},           // 56
      {{48.867584, 2.353219}, {48.867617, 2.353077}, routing::VehicleType::Pedestrian},      // 57
      {{48.868294, 2.323534}, {48.868426, 2.323615}, routing::VehicleType::Pedestrian},      // 58
      {{48.872028, 2.359879}, {48.872093, 2.360042}, routing::VehicleType::Car},             // 59
      {{49.550927, 25.593832}, {49.551146, 25.593766}, routing::VehicleType::Car},           // 60
      {{49.564538, 25.633862}, {49.564365, 25.634210}, routing::VehicleType::Car},           // 61
      {{49.675343, -125.010402}, {49.675095, -125.010445}, routing::VehicleType::Car},       // 62
      {{50.128494, 5.342730}, {50.128312, 5.342838}, routing::VehicleType::Pedestrian},      // 63
      {{50.293157, 57.153277}, {50.292945, 57.153430}, routing::VehicleType::Car},           // 64
      {{50.424169, 30.543769}, {50.424075, 30.543552}, routing::VehicleType::Car},           // 65
      {{50.456769, 3.699087}, {50.456896, 3.698898}, routing::VehicleType::Car},             // 66
      {{50.513276, 30.493644}, {50.513395, 30.493749}, routing::VehicleType::Pedestrian},    // 67
      {{50.948447, 6.943709}, {50.948523, 6.943618}, routing::VehicleType::Pedestrian},      // 68
      {{51.070450, 13.690914}, {51.070186, 13.690486}, routing::VehicleType::Car},           // 69
      {{51.442925, 5.475704}, {51.442785, 5.475484}, routing::VehicleType::Bicycle},         // 70
      {{52.210605, 104.301558}, {52.210766, 104.299210}, routing::VehicleType::Pedestrian},  // 71
      {{53.132201, 17.992780}, {53.132109, 17.992736}, routing::VehicleType::Pedestrian},    // 72
      {{53.450367, 26.471001}, {53.451092, 26.473267}, routing::VehicleType::Car},           // 73
      {{53.544199, 9.942703}, {53.544226, 9.943041}, routing::VehicleType::Pedestrian},      // 74
      {{53.663670, 23.809416}, {53.663587, 23.809148}, routing::VehicleType::Car},           // 75
      {{53.688767, 23.840653}, {53.688625, 23.840767}, routing::VehicleType::Car},           // 76
      {{53.705198, 23.799942}, {53.705020, 23.799729}, routing::VehicleType::Car},           // 78
      {{53.721229, 23.853621}, {53.721089, 23.853499}, routing::VehicleType::Car},           // 79
      {{53.908905, 27.492089}, {53.908688, 27.492657}, routing::VehicleType::Car},           // 80
      {{53.915279, 27.497204}, {53.915298, 27.497692}, routing::VehicleType::Car},           // 81
      {{54.708002, 20.588852}, {54.709596, 20.589344}, routing::VehicleType::Pedestrian},    // 82
      {{56.328089, 43.780187}, {56.329598, 43.779347}, routing::VehicleType::Pedestrian},    // 83
      {{56.328148, 43.780259}, {56.329598, 43.779347}, routing::VehicleType::Pedestrian},    // 84
      {{60.601636, 27.840518}, {60.601707, 27.841084}, routing::VehicleType::Car},           // 85
      {{60.602799, 27.838590}, {60.602779, 27.839252}, routing::VehicleType::Car},           // 86
      {{61.002506, 72.586208}, {61.002588, 72.586563}, routing::VehicleType::Car},           // 87
      {{61.311170, 63.318459}, {61.310999, 63.318328}, routing::VehicleType::Car},           // 88
      {{61.460908, 76.638827}, {61.460768, 76.639447}, routing::VehicleType::Car},           // 89
      {{63.370116, 10.360256}, {63.370199, 10.360618}, routing::VehicleType::Car},           // 90
      {{63.975020, -22.575718}, {63.975126, -22.576316}, routing::VehicleType::Car},         // 91
      {{64.538104, -21.926846}, {64.538149, -21.927736}, routing::VehicleType::Car},         // 92
      {{64.538306, 40.520611}, {64.538233, 40.520327}, routing::VehicleType::Car},           // 93
      {{9.625079, 123.805457}, {9.625084, 123.805655}, routing::VehicleType::Car},           // 94
  };

  std::vector<std::tuple<ms::LatLon, ms::LatLon, m2::PointD, routing::VehicleType>> routesWithDir = {
      {{-45.433213, -72.739150},
       {-45.434484, -72.738892},
       {-1.3387623880589671899e-06, -4.2558102819612031453e-07},
       routing::VehicleType::Car},  // 1
  };

  size_t number = 0;
  for (auto const & route : routes)
  {
    ++number;
    ms::LatLon start;
    ms::LatLon finish;
    routing::VehicleType type;
    std::tie(start, finish, type) = route;

    LOG(LINFO, ("Start test without direction, number:", number));
    TRouteResult result = integration::CalculateRoute(
        integration::GetVehicleComponents(type), mercator::FromLatLon(start), {0., 0.}, mercator::FromLatLon(finish));
    TEST_EQUAL(result.second, routing::RouterResultCode::NoError,
               (std::get<0>(route), std::get<1>(route), std::get<2>(route)));
  }

  number = 0;
  for (auto const & route : routesWithDir)
  {
    ++number;
    ms::LatLon start;
    ms::LatLon finish;
    routing::VehicleType type;
    m2::PointD direction;
    std::tie(start, finish, direction, type) = route;

    LOG(LINFO, ("Start test with direction, number:", number));
    TRouteResult result = integration::CalculateRoute(
        integration::GetVehicleComponents(type), mercator::FromLatLon(start), direction, mercator::FromLatLon(finish));
    TEST_EQUAL(result.second, routing::RouterResultCode::NoError, ());
  }
}
}  // namespace
