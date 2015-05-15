#include "testing/testing.hpp"

#include "integration_tests/osrm_test_tools.hpp"

#include "routing/route.hpp"


using namespace routing;
using namespace routing::turns;

UNIT_TEST(RussiaMoscowLenigradskiy39UturnTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.537544383032568, 67.536216737893028}, {0., 0.},
      {37.538908531885973, 67.54544090660923});

  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, OsrmRouter::NoError, ());

  integration::TestTurnCount(route, 3);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestOneOfDirections({TurnDirection::TurnSlightLeft, TurnDirection::TurnLeft});
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(TurnDirection::TurnRight);
  integration::GetNthTurn(route, 2).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 2033.);
}

UNIT_TEST(RussiaMoscowSalameiNerisUturnTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.395332276656617, 67.633925439079519}, {0., 0.},
      {37.392503720352721, 67.61975260731343});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 4);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.388482521388539, 67.633382734905041}, 5.)
      .TestDirection(TurnDirection::TurnSlightRight);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestPoint({37.387117276989784, 67.633369323859881})
      .TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 2)
      .TestValid()
      .TestPoint({37.387380133475205, 67.632781920081243})
      .TestDirection(TurnDirection::TurnLeft);
  integration::GetNthTurn(route, 3)
      .TestValid()
      .TestPoint({37.390526364673121, 67.633106467374461})
      .TestDirection(TurnDirection::TurnRight);

  integration::TestRouteLength(route, 1637.);
}

UNIT_TEST(RussiaMoscowTrikotagniAndPohodniRoundaboutTurnTest)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetAllMaps(), {37.405153751040686, 67.5971698246356},
                                  {0., 0.}, {37.40521071657038, 67.601903779043795});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 2);

  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(TurnDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(2);
  integration::GetNthTurn(route, 1).TestValid().TestDirection(TurnDirection::LeaveRoundAbout);

  integration::TestRouteLength(route, 387.);
}

UNIT_TEST(RussiaMoscowPlanetnaiTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.546683164991776, 67.545511147376089}, {0., 0.},
      {37.549153861529007, 67.54467404790482});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);

  integration::TestRouteLength(route, 214.);
}

UNIT_TEST(RussiaMoscowNoTurnsOnMKADTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.391635636579785, 67.62455792789649}, {0., 0.},
      {37.692547253527685, 67.127684414191762});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestPoint({37.682761085650043, 67.140620702062705})
      .TestOneOfDirections({TurnDirection::TurnSlightRight, TurnDirection::TurnRight});

  integration::TestRouteLength(route, 43233.7);
}

UNIT_TEST(RussiaMoscowTTKKashirskoeShosseOutTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.606320023648998, 67.36682695403141}, {0., 0.},
      {37.621220025471168, 67.352441627022912});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  // Checking a turn in case going from not a link to a link
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPankratevskiPerBolshaySuharedskazPloschadTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.635563528539393, 67.491460725721268}, {0., 0.},
      {37.637054339197832, 67.491929797067371});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnRight);
}

UNIT_TEST(RussiaMoscowMKADPutilkovskeShosseTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.394141645624103, 67.63612831787222}, {0., 0.},
      {37.391050708989461, 67.632454269643091});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

UNIT_TEST(RussiaMoscowPetushkovaShodniaReverTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.405917692164508, 67.614731601278493}, {0., 0.},
      {37.408550782937482, 67.61160397953185});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 0);
}

UNIT_TEST(RussiaHugeRoundaboutTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.325810690728495, 67.544175542376436}, {0., 0.},
      {37.325360456262153, 67.543013703414516});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 2);
  integration::GetNthTurn(route, 0)
      .TestValid()
      .TestDirection(TurnDirection::EnterRoundAbout)
      .TestRoundAboutExitNum(4);
  integration::GetNthTurn(route, 1)
      .TestValid()
      .TestDirection(TurnDirection::LeaveRoundAbout)
      .TestRoundAboutExitNum(0);
}

UNIT_TEST(BelarusMiskProspNezavisimostiMKADTurnTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {27.658572046123947, 64.302533720228126}, {0., 0.},
      {27.670461944729382, 64.307480201489582});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::TurnSlightRight, TurnDirection::TurnRight});
}

// Test case: turning form one street to another one with the same name.
// An end user shall be informed about this manoeuvre.
UNIT_TEST(RussiaMoscowPetushkovaPetushkovaTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.405555547431582, 67.606396877452852}, {0., 0.},
      {37.404889653177761, 67.607659749718096});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestDirection(TurnDirection::TurnLeft);
}

// Test case: a route goes straightly along a unnamed big link road when joined a small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(RussiaMoscowMKADLeningradkaTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {37.438582086802406, 67.683581448033891}, {0., 0.},
      {37.448852064651611, 67.681148387254225});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
}

UNIT_TEST(BelarusMKADShosseinai)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {29.431229709918465, 66.684863888688056}, {0., 0.},
      {29.426264439664614, 66.686867924003238});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 1);
  integration::GetNthTurn(route, 0).TestValid().TestOneOfDirections(
      {TurnDirection::GoStraight, TurnDirection::TurnSlightRight});
}

// Test case: a route goes straightly along a unnamed big road when joined small road.
// An end user shall not be informed about such manoeuvres.
UNIT_TEST(ThailandPhuketNearPrabarameeRoad)
{
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetAllMaps(), {98.369368627299635, 7.9432901571200318}, {0., 0.},
      {98.367845572071232, 7.9324732487227401});
  Route const & route = *routeResult.first;
  OsrmRouter::ResultCode const result = routeResult.second;

  TEST_EQUAL(result, OsrmRouter::NoError, ());
  integration::TestTurnCount(route, 0);
}
