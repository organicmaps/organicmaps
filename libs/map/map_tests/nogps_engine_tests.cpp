#include "testing/testing.hpp"

#include "map/map_tests/nogps_test_env.hpp"

#include "map/nogps/engine.hpp"
#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/geo.hpp"
#include "map/nogps/storage.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace nogps_engine_tests
{
using namespace nogps;
using namespace nogps_test;

int constexpr kCalibratedFlags = esp32::kFlagImuOk | esp32::kFlagBiasOk | esp32::kFlagUpOk | esp32::kFlagObdOk;

class Env
{
public:
  /// \param roads the roads instead of the grid of streets.
  explicit Env(bool inertial, Roads * roads = nullptr)
  {
    // The inertial navigation takes the roads when it starts.
    m_map.m_roads = roads;
    m_storage.Set(Storage::kInertialEnabled, inertial);
    m_engine.Start();
    ConnectBox();
  }

  /// The platform has found the box and has connected to it.
  void ConnectBox() { m_engine.OnEsp32BleState(BleState::Connected); }

  /// The ESP32 box sends its data 50 times a second for |ms|: the car drives at |speedKmh| turning at |yawRateDegS|,
  /// clockwise is positive.
  void Drive(int64_t ms, int speedKmh, double yawRateDegS, int flags = kCalibratedFlags)
  {
    for (int64_t t = 0; t < ms; t += 20)
    {
      m_clock.Advance(20);
      m_boxTimeMs += 20;
      m_yawMdeg += yawRateDegS * 20;
      m_engine.OnEsp32BleBytes(BoxDataLine(++m_seq, m_boxTimeMs, std::llround(m_yawMdeg), speedKmh, flags));
    }
  }

  /// GPS positions every second for |seconds| at |speedMps| from the point, going east along the street.
  void DriveGps(int seconds, double eastM, double speedMps)
  {
    for (int i = 0; i < seconds; ++i)
    {
      m_clock.Advance(1000);
      m_engine.OnFix(GpsAt(m_clock, eastM + i * speedMps, 0, 90.0, speedMps));
    }
  }

  /// The box starts again: it counts its lines, time and rotation from the start.
  void RestartBox()
  {
    m_seq = 0;
    m_boxTimeMs = 0;
    m_yawMdeg = 0;
  }

  /// The box sends nothing, |ms| pass by its clock.
  void SleepBox(int64_t ms) { m_boxTimeMs += ms; }

  Fix const & Last() const { return m_delegate.m_positions.back(); }

  TestClock m_clock;
  TestStorage m_storage;
  TestDelegate m_delegate;
  TestMap m_map;
  Engine m_engine{m_delegate, m_map, m_storage, m_clock, m_clock};

private:
  int64_t m_seq = 0;
  int64_t m_boxTimeMs = 100'000;
  double m_yawMdeg = 0;
};

void TestAlmostEqualAbs(double actual, double expected, double eps)
{
  TEST_ALMOST_EQUAL_ABS(actual, expected, eps, ());
}

// The car is marked on the street going east, the box tells it stands.
void MarkCar(Env & env, double eastM)
{
  env.m_engine.SetManualMode(true);
  env.Drive(500, 0, 0);
  TEST(env.m_engine.PlaceMarkByTap(At(eastM, 3)), ());
  TEST_EQUAL(env.Last().m_provider, Provider::Manual, ());
  TestAlmostEqualAbs(East(env.Last().m_position), eastM, 0.5);
}

/// The engine of a user who has set nothing but what the test saves before the start.
struct NewUser
{
  nogps::Status Start()
  {
    m_engine.Start();
    return m_engine.GetStatus();
  }

  TestClock m_clock;
  TestStorage m_storage;
  TestDelegate m_delegate;
  TestMap m_map;
  Engine m_engine{m_delegate, m_map, m_storage, m_clock, m_clock};
};

UNIT_TEST(NoGps_Engine_LooksForBoxOverBluetoothOutOfTheBox)
{
  {
    // Nothing is chosen: the box is found without touching the settings.
    NewUser user;
    auto const status = user.Start();
    TEST(status.m_inertialEnabled, ());
    TEST(status.m_esp32Source, ());
    TEST(user.m_delegate.m_esp32BleOpen, ());
    TEST(user.m_delegate.m_elm327Connects.empty(), ());
  }
  {
    // The user has switched the navigation off.
    NewUser user;
    user.m_storage.Set(Storage::kInertialEnabled, false);
    TEST(!user.Start().m_inertialEnabled, ());
    TEST(!user.m_delegate.m_esp32BleOpen, ());
    TEST(!user.m_delegate.m_sensorsStarted, ());
  }
  {
    // The adapter was chosen when the phone was the default source: it stays.
    NewUser user;
    user.m_storage.Set(Storage::kInertialEnabled, true);
    user.m_storage.Set(Storage::kElm327Address, std::string("00:11:22:33:44:55"));
    TEST(!user.Start().m_esp32Source, ());
    TEST(!user.m_delegate.m_esp32BleOpen, ());
    TEST_EQUAL(user.m_delegate.m_elm327Connects.size(), 1, ());
  }
  {
    // The box was reached over its Wi-Fi, which it has no more: it is found over Bluetooth.
    NewUser user;
    user.m_storage.Set(Storage::kInertialEnabled, true);
    user.m_storage.Set(Storage::kEsp32Source, true);
    user.m_storage.SetString("NoGpsEsp32Bluetooth", "false");
    TEST(user.Start().m_esp32Source, ());
    TEST(user.m_delegate.m_esp32BleOpen, ());
  }
}

UNIT_TEST(NoGps_Engine_StartsTheBox)
{
  Env env(true /* inertial */);
  TEST(env.m_delegate.m_esp32BleOpen, ());
  // The accelerometer goes to the trip log, the gyroscope is in the box.
  TEST(env.m_delegate.m_sensorsStarted, ());
  TEST(!env.m_delegate.m_gyroscope, ());
  env.Drive(500, 0, 0);
  auto const status = env.m_engine.GetStatus();
  TEST(status.m_inertialStarted, ());
  TEST(status.m_esp32Source, ());
  TEST_EQUAL(status.m_sourceState, SourceState::Connected, ());
  TEST_EQUAL(status.m_calibration, CalibrationState::Done, ());
  TEST_EQUAL(status.m_speedKmh, 0, ());

  env.m_engine.Stop();
  TEST(!env.m_delegate.m_esp32BleOpen, ());
  TEST(!env.m_delegate.m_sensorsStarted, ());
}

UNIT_TEST(NoGps_Engine_MarkIsOnRoadAndShowsCarHeading)
{
  Env env(false /* inertial */);
  env.m_engine.SetManualMode(true);
  TEST(env.m_delegate.HasEvent(Event::ManualModeChanged), ());
  // A tap between the streets is not where the car is.
  TEST(!env.m_engine.PlaceMarkByTap(At(50, 50)), ());
  TEST(env.m_delegate.HasEvent(Event::MarkNoRoad), ());
  TEST(env.m_engine.PlaceMarkByTap(At(60, 4)), ());
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);
  TEST_EQUAL(env.Last().m_bearingDeg, 90.0, ());
  TEST(env.m_engine.IsCarHeadingShown(), ());
  TEST_EQUAL(env.m_map.m_carHeading, 90.0, ());
  auto const status = env.m_engine.GetStatus();
  TEST_EQUAL(status.m_source, PositionSource::Manual, ());
  TEST(status.m_movedByHand, ());

  // The mark is sent again and again, otherwise the position is treated as lost.
  size_t const positions = env.m_delegate.m_positions.size();
  env.m_clock.Advance(Engine::kManualRepeatIntervalMs);
  TEST_EQUAL(env.m_delegate.m_positions.size(), positions + 1, ());

  env.m_engine.ReverseDirection();
  TEST_EQUAL(env.Last().m_bearingDeg, 270.0, ());
}

UNIT_TEST(NoGps_Engine_ShiftStopsAtCrossing)
{
  Env env(false /* inertial */);
  env.m_engine.SetManualMode(true);
  TEST(env.m_engine.PlaceMarkByTap(At(60, 2)), ());
  // There is no route, the position is moved along the road up to the crossing.
  TestAlmostEqualAbs(env.m_engine.ShiftPosition(100), 40, 0.5);
  TestAlmostEqualAbs(East(env.Last().m_position), 100, 0.5);
  // The car may turn at the crossing: it is not moved further that way, but it is moved back.
  TEST(env.m_engine.IsShiftBlocked(true /* forward */), ());
  TEST(!env.m_engine.IsShiftBlocked(false /* forward */), ());
  TEST_EQUAL(env.m_engine.ShiftPosition(50), 0, ());
  TestAlmostEqualAbs(env.m_engine.ShiftPosition(-30), -30, 0.5);
  TestAlmostEqualAbs(East(env.Last().m_position), 70, 0.5);
  TEST(!env.m_engine.IsShiftBlocked(true /* forward */), ());
}

UNIT_TEST(NoGps_Engine_CyclesShiftSteps)
{
  Env env(false /* inertial */);
  TEST_EQUAL(env.m_engine.GetStatus().m_shiftStepM, 50, ());
  env.m_engine.CycleShiftStep();
  TEST_EQUAL(env.m_engine.GetStatus().m_shiftStepM, 100, ());
  env.m_engine.CycleShiftStep();
  TEST_EQUAL(env.m_engine.GetStatus().m_shiftStepM, 10, ());
}

// The value of a field of the trip line, e.g. "turn".
std::string TripField(Env & env, std::string const & name)
{
  std::string const line = env.m_engine.GetTripLine();
  auto const key = line.find(" " + name + "=");
  TEST(key != std::string::npos, (name, line));
  auto const from = key + name.size() + 2;
  return line.substr(from, line.find(' ', from) - from);
}

// The car driving 10 m/s east turns left at the crossing at 200 m, the calculated position lags behind it: it is
// |beforeTurnM| east when the turn starts. Then the car drives north.
void TurnLeftAtCrossing(Env & env, double beforeTurnM)
{
  // The position is 112 m farther after 10 s.
  MarkCar(env, beforeTurnM - 112);
  env.Drive(10'000, 36, 0);
  TEST_EQUAL(env.Last().m_provider, Provider::Inertial, ());
  TestAlmostEqualAbs(East(env.Last().m_position), beforeTurnM, 2);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 1);
  TEST_EQUAL(TripField(env, "turn"), "NaN", ());
  env.Drive(3000, 36, -30);
  env.Drive(3000, 36, 0);
  // The car is on the street going north from the crossing.
  TestAlmostEqualAbs(East(env.Last().m_position), 200, 1);
  TEST_GREATER(North(env.Last().m_position), 20, ());
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 0), 0, 3);
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_InertialTurnIsMovedToCrossing)
{
  Env env(true /* inertial */);
  // The car starts turning 5 m before the middle of a crossing: this one is 18 m farther.
  TurnLeftAtCrossing(env, 177);
  TestAlmostEqualAbs(std::stod(TripField(env, "turn")), 18, 2);
  // The crossing is a known place: the error of the calculated distance is gone, the car has driven 20 m since.
  TEST_LESS(env.Last().m_accuracyM, DeadReckoning::kBaseAccuracyM + 1, ());
}

UNIT_TEST(NoGps_Engine_InertialTurnFarFromCrossingIsNotMovedToIt)
{
  Env env(true /* inertial */);
  // The crossing is 33 m farther than the turn, and a crossing is searched within 25 m: farther away another
  // crossing may be the one. The street going north is 14 m aside of the car after the turn, the car is on it
  // as on any road it drives along.
  TurnLeftAtCrossing(env, 162);
  TEST_EQUAL(TripField(env, "turn"), "NaN", ());
  // Its place along the street is not known better than before the turn.
  TEST_GREATER(env.Last().m_accuracyM, DeadReckoning::kBaseAccuracyM + 2, ());
}

UNIT_TEST(NoGps_Engine_TurnOfCarManeuveringOffRoadsIsNotMoved)
{
  for (bool twoStreets : {true, false})
  {
    // A street going east. 30 m to the south of it another one going east crosses a street going south, or two of
    // them 30 m apart.
    std::vector<PieceRoads::Piece> pieces = {{-300, 0, 300, 0}, {-100, -30, 100, -30}, {22, -10, 22, -60}};
    if (twoStreets)
      pieces.push_back({-8, -10, -8, -60});
    PieceRoads roads(pieces, {{-8, -30}, {22, -30}});
    Env env(true /* inertial */, &roads);
    MarkCar(env, 0);
    // The car leaves the street to the south at a walking speed, between the streets going south and 15 m from
    // each, drives 10 m and turns east.
    env.Drive(2000, 9, 0);
    env.Drive(1000, 9, 90);
    env.Drive(4000, 9, 0);
    TEST(env.m_engine.GetTripLine().find(" road=0 ") != std::string::npos, (env.m_engine.GetTripLine()));
    env.Drive(1000, 9, -90);
    env.Drive(2000, 9, 0);
    // The car was maneuvering off the roads: it has turned where it is, be there two crossings to turn at or one.
    TEST_EQUAL(TripField(env, "turn"), "NaN", (twoStreets));
  }

  // A turn as slow of a car driving along a street is moved to the crossing of that street, 12 m ahead.
  PieceRoads roads({{-300, 0, 300, 0}, {40, -300, 40, 300}}, {{40, 0}});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  env.Drive(8000, 9, 0);
  TEST(env.m_engine.GetTripLine().find(" road=1 ") != std::string::npos, (env.m_engine.GetTripLine()));
  env.Drive(3000, 9, -30);
  env.Drive(2000, 9, 0);
  TestAlmostEqualAbs(std::stod(TripField(env, "turn")), 12, 4);
  TestAlmostEqualAbs(East(env.Last().m_position), 40, 1);
}

UNIT_TEST(NoGps_Engine_TurnAlongBranchIsNotMovedToNextCrossing)
{
  // A road branches off the street going east to the north-east at 100 m, and a street goes the same way from
  // the crossing at 115 m, 13 m aside of the branch. The branch leaves the street not at a crossing of the map.
  double const sin30 = 0.5;
  double const cos30 = std::sqrt(0.75);
  PieceRoads roads(
      {{-300, 0, 500, 0}, {100, 0, 100 + 300 * sin30, 300 * cos30}, {115, 0, 115 + 300 * sin30, 300 * cos30}},
      {{115, 0}});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  // 10 m/s east, then the car takes the branch, 60 degrees to the left: the turn of 19 m radius starts 11 m
  // before the branch.
  env.Drive(7700, 36, 0);
  TEST_EQUAL(env.Last().m_provider, Provider::Inertial, ());
  TestAlmostEqualAbs(East(env.Last().m_position), 89, 3);
  env.Drive(2000, 36, -30);
  env.Drive(4000, 36, 0);
  // The car is on the branch, not on the street going the same way from the crossing.
  auto const & position = env.Last().m_position;
  double const fromBranchM = (East(position) - 100) * cos30 - North(position) * sin30;
  TestAlmostEqualAbs(fromBranchM, 0, 3);
  TEST_GREATER(North(position), 30, ());
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_CarAtRoundaboutIsNotLost)
{
  // The street the car leaves the roundabout by is where the car is, or 30 m to the left of it: the car came to
  // the roundabout with its distance wrong by that much.
  for (auto const & [roundabout, exitNorth] :
       std::vector<std::pair<bool, double>>{{true, 7.5}, {false, 7.5}, {true, 37.5}, {false, 37.5}})
  {
    // A street going east comes to a roundabout and leaves it to the north of where it came. The mark doesn't
    // follow the circle of the map: here the circle has no pieces at all.
    PieceRoads roads({{-300, 0, -30, 0}, {75, exitNorth, 400, exitNorth}}, {});
    if (roundabout)
      roads.AddRoundabout(15, 0, 25);
    Env env(true /* inertial */, &roads);
    MarkCar(env, -100);
    env.Drive(8000, 27, 0);
    TEST(env.m_engine.GetTripLine().find(" road=1 ") != std::string::npos, (env.m_engine.GetTripLine()));
    // The car drives 7.5 m/s around the circle: to the right, to the left by 70 degrees, straight for 1.5 s, which
    // ends its turn, and slowly to the right for 37 m, slower than a turn begins.
    env.Drive(1000, 27, 45);
    env.Drive(2000, 27, -35);
    env.Drive(1500, 27, 0);
    env.Drive(2500, 27, 5);
    std::string const line = env.m_engine.GetTripLine();
    TEST(line.find(" road=0 ") != std::string::npos, (line));
    TEST_EQUAL(line.find(" ring=1") != std::string::npos, roundabout, (line));
    env.Drive(2500, 27, 5);
    env.Drive(4000, 27, 0);
    // Without the roundabout a car driving so far off the roads is lost and stopped.
    TEST_EQUAL(env.m_delegate.HasEvent(Event::RoadLost), !roundabout, ());
    if (!roundabout)
      continue;
    // The car is on the street it has left the roundabout by.
    TEST(!env.m_engine.GetStatus().m_paused, ());
    std::string const after = env.m_engine.GetTripLine();
    TEST(after.find(" road=1 ") != std::string::npos, (after));
    TEST(after.find(" ring=0") != std::string::npos, (after));
    TestAlmostEqualAbs(North(env.Last().m_position), exitNorth, 1);
    TEST_GREATER(East(env.Last().m_position), 75, ());
    TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 90), 0, 3);
  }
}

UNIT_TEST(NoGps_Engine_StopsCarThatLeftRoads)
{
  Env env(true /* inertial */);
  MarkCar(env, 50);
  // The heading goes wrong slowly, slower than a turn: the car leaves the road.
  env.Drive(12'000, 36, -5);
  env.Drive(5000, 36, 0);
  TEST(env.m_delegate.HasEvent(Event::RoadLost), ());
  // The car stands where it was on the road the last time until the user marks it.
  auto const stopped = env.Last().m_position;
  env.Drive(2000, 36, 0);
  TestAlmostEqualAbs(Distance(env.Last().m_position, stopped), 0, 0.01);
  TEST(env.m_engine.PlaceMarkByTap(At(300, 0)), ());
  env.Drive(1000, 36, 0);
  TEST_GREATER(East(env.Last().m_position), 300, ());
}

UNIT_TEST(NoGps_Engine_MarkLooksAlongRoadWhateverGyroscopeCounted)
{
  // The street going east, a street going north crosses it at -100 m.
  PieceRoads roads({{-300, 0, 600, 0}, {-100, -300, -100, 300}}, {{-100, 0}});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  env.Drive(2000, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 1);

  // The heading has gone 18 degrees aside of the street the car drives along: the car has left a parking place,
  // and its turns were taken for the turns of the road. The mark is drawn across the street.
  env.Drive(1000, 18, 18);
  env.Drive(4000, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 108, 1);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 2);

  // The user marks the car on the street while it drives: it looks along the street at once, and keeps looking.
  double const east = East(env.Last().m_position);
  TEST(env.m_engine.PlaceMarkByTap(At(east + 10, 2)), ());
  env.Drive(InertialNavigator::kOutputIntervalMs, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.01);
  TestAlmostEqualAbs(*env.m_map.m_carHeading, 90, 0.01);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.01);
  env.Drive(2000, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.01);
  TestAlmostEqualAbs(East(env.Last().m_position), east + 10 + 10.5, 4);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.01);

  // The same with a heading 25 degrees to the other side, and the way of the street is the one the car goes.
  env.Drive(1000, 18, -25);
  env.Drive(InertialNavigator::kOutputIntervalMs, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 65, 3);
  TEST(env.m_engine.PlaceMarkByTap(At(East(env.Last().m_position), -2)), ());
  env.Drive(InertialNavigator::kOutputIntervalMs, 18, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.01);
  // It is turned around by the button only.
  env.m_engine.ReverseDirection();
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 270, 0.01);
  env.m_engine.ReverseDirection();
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.01);

  // A mark on the crossing street: the car has turned to it and looks along it.
  env.Drive(1000, 0, 0);
  TEST(env.m_engine.PlaceMarkByTap(At(-100, 100)), ());
  env.Drive(500, 0, 0);
  TestAlmostEqualAbs(East(env.Last().m_position), -100, 1);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 0), 0, 1);
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_BranchOfForkIsToldByHeading)
{
  // The street going east forks at 100 m: a branch goes on east, the other one leaves 5 degrees to the right. The
  // route the car follows goes on east.
  double const tan5 = std::tan(math::DegToRad(5.0));
  PieceRoads roads({{-300, 0, 100, 0}, {100, 0, 700, 0}, {100, 0, 700, -600 * tan5}}, {{100, 0}});
  roads.SetRoutePiece(1);
  for (bool right : {false, true})
  {
    Env env(true /* inertial */, &roads);
    MarkCar(env, 0);
    // 15 m/s: the car passes the fork in the seventh second, keeping east or turning the 5 degrees.
    env.Drive(6000, 54, 0);
    env.Drive(1000, 54, right ? 5 : 0);
    env.Drive(8000, 54, 0);
    // The route takes a mark that is as near to it as to its road, and here it is so for the first seconds after
    // the fork. The car that has turned is on the branch it has turned to nevertheless.
    double const east = East(env.Last().m_position);
    TEST_GREATER(east, 200, ());
    TestAlmostEqualAbs(North(env.Last().m_position), right ? -(east - 100) * tan5 : 0, 1);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, right ? 95 : 90, 1);
    TEST(!env.m_delegate.HasEvent(Event::RoadLost), (right));
  }
}

UNIT_TEST(NoGps_Engine_MarkSetDuringTurnKeepsLookingAlongRoad)
{
  // The street going east, a street going north crosses it at 100 m, and another street going east 200 m to the
  // north.
  PieceRoads roads({{-300, 0, 600, 0}, {100, -300, 100, 600}, {-300, 200, 600, 200}}, {{100, 0}, {100, 200}});
  for (int standMs : {0, 3000})
  {
    Env env(true /* inertial */, &roads);
    MarkCar(env, 0);
    env.Drive(6000, 36, 0);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 1);

    // The car turns left to the street going north. In the middle of the turn, looking north-east, the user
    // marks it on that street: the mark looks north.
    env.Drive(1500, 18, -30);
    TEST(env.m_engine.GetTripLine().find(" turning=1 ") != std::string::npos, (env.m_engine.GetTripLine()));
    TEST(env.m_engine.PlaceMarkByTap(At(100, 15)), ());
    // It may creep on and stand for a while, e.g. letting a car pass, and makes the other half of the turn.
    if (standMs > 0)
      env.Drive(1500, 2, 0);
    env.Drive(standMs, 0, 0);
    env.Drive(1500, 18, -30);
    env.Drive(3000, 18, 0);
    // The mark was not turned away from the street by the rest of the turn.
    TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 0), 0, 2);
    TestAlmostEqualAbs(East(env.Last().m_position), 100, 0.5);
    TEST_GREATER(North(env.Last().m_position), 30, (standMs));
    TEST(env.m_engine.GetTripLine().find(" road=1 ") != std::string::npos, (env.m_engine.GetTripLine()));

    // The next turn is a turn as any other: to the right, to the other street going east.
    env.Drive(31'000, 18, 0);
    env.Drive(3000, 18, 30);
    env.Drive(3000, 18, 0);
    TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 90), 0, 3);
    TestAlmostEqualAbs(North(env.Last().m_position), 200, 1);
    TEST(!env.m_delegate.HasEvent(Event::RoadLost), (standMs));
  }

  // A car turning right is marked on a street that goes farther to the left than it looks: it is not the turn
  // the car is making, the heading was wrong. The mark looks along the street, and a turn right after it is a turn.
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  env.Drive(6000, 36, 0);
  env.Drive(1500, 18, 30);
  TEST(env.m_engine.PlaceMarkByTap(At(100, 15)), ());
  env.Drive(500, 18, 0);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 0), 0, 1);
  env.Drive(1500, 18, -30);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, -45), 0, 3);
}

UNIT_TEST(NoGps_Engine_MarkAheadOnBendComesBackToRoad)
{
  // The street going east bends 25 degrees to the left at 100 m.
  double const sin65 = std::sin(math::DegToRad(65.0));
  double const cos65 = std::cos(math::DegToRad(65.0));
  PieceRoads roads({{-300, 0, 100, 0}, {100, 0, 100 + 600 * sin65, 600 * cos65}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 30);
  env.Drive(4000, 36, 0);
  env.Drive(1000, 0, 0);
  TestAlmostEqualAbs(East(env.Last().m_position), 82, 3);

  // The car stands 20 m before the bend, the user taps 15 m behind it: the mark is where the user has put it and
  // looks along the street there.
  TEST(env.m_engine.PlaceMarkByTap(At(100 + 15 * sin65, 15 * cos65)), ());
  env.Drive(500, 0, 0);
  TestAlmostEqualAbs(North(env.Last().m_position), 15 * cos65, 1);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 65, 0.5);

  // The car drives through the bend the mark has passed already: the mark looks 25 degrees aside of the street, is
  // kept on it, and looks along it again in 50 m.
  env.Drive(2000, 36, 0);
  env.Drive(1000, 36, -25);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 40, 3);
  env.Drive(30000, 36, 0);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 65, 3);
  auto const & position = env.Last().m_position;
  TestAlmostEqualAbs((East(position) - 100) * cos65 - North(position) * sin65, 0, 1);
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_TellsThatMovedBoxDoesNotFollowCar)
{
  Env env(true /* inertial */);
  MarkCar(env, 50);
  env.Drive(5000, 36, 0);
  TEST_GREATER(East(env.Last().m_position), 90, ());
  TEST(!env.m_delegate.HasEvent(Event::NotCalibrated), ());

  // The box tells it has been moved in the car: its rotation is not trusted, the car is not followed.
  auto const stopped = env.Last().m_position;
  int const movedFlags = kCalibratedFlags | esp32::kFlagMountMoved;
  env.Drive(Engine::kNotCalibratedDelayMs - 1500, 36, 0, movedFlags);
  // Not at once: the box needs a couple of seconds after it wakes up.
  TEST(!env.m_delegate.HasEvent(Event::NotCalibrated), ());
  env.Drive(4000, 36, 0, movedFlags);
  TEST(env.m_delegate.HasEvent(Event::NotCalibrated), ());
  TestAlmostEqualAbs(Distance(env.Last().m_position, stopped), 0, 0.01);

  // The user is told once in a while, not every second, and not while the car stands.
  auto const count = [&env]
  { return std::count(env.m_delegate.m_events.begin(), env.m_delegate.m_events.end(), Event::NotCalibrated); };
  TEST_EQUAL(count(), 1, ());
  env.Drive(Engine::kMotionSourceWarningIntervalMs, 36, 0, movedFlags);
  TEST_EQUAL(count(), 2, ());
  env.Drive(Engine::kMotionSourceWarningIntervalMs + 5000, 0, 0, movedFlags);
  TEST_EQUAL(count(), 2, ());

  // Calibrated again: the car is followed.
  env.Drive(3000, 36, 0);
  TEST_GREATER(Distance(env.Last().m_position, stopped), 10, ());
}

UNIT_TEST(NoGps_Engine_AdvisesToCalibratePluggedBox)
{
  Env env(true /* inertial */);
  auto const count = [&env]
  { return std::count(env.m_delegate.m_events.begin(), env.m_delegate.m_events.end(), Event::BoxCalibrationAdvised); };
  auto const advised = [&env] { return env.m_engine.GetStatus().m_boxCalibrationAdvised; };
  // The first event of the journal of the box: how it has started.
  auto const boot = [&env](std::string const & reason)
  { env.m_engine.OnEsp32BleBytes(BoxLine("NGE,1,1,40,I,BOOT,fw 0.3.3 reset " + reason)); };
  // The box tells about itself, its firmware doesn't count its power-ons.
  auto const info = [&env]
  {
    env.m_engine.OnEsp32BleBytes(
        BoxLine("NGI,1,0.3.3,2026-10-10T07:33,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA"));
  };

  // The box was plugged into the car: it may stand in another way than it was calibrated.
  env.Drive(2000, 0, 0);
  boot("POWERON");
  env.Drive(2000, 0, 0);
  // Not before the box tells what it is.
  TEST_EQUAL(count(), 0, ());
  TEST(!advised(), ());
  info();
  env.Drive(2000, 0, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(advised(), ());
  // The user is told once, the status tells it until the box is calibrated.
  env.Drive(5000, 0, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(advised(), ());

  env.m_engine.Calibrate();
  std::string const command = env.m_delegate.m_esp32BleSent.back();
  auto const idEnd = command.find(",CAL_UP*");
  TEST(idEnd != std::string::npos, (command));
  env.m_engine.OnEsp32BleBytes(BoxLine("NGA," + command.substr(5, idEnd - 5) + ",OK"));
  env.Drive(2000, 0, 0);
  TEST(!advised(), ());

  // The journal of the box is read again: the same power-on.
  env.m_engine.SetEsp32Source(true);
  env.ConnectBox();
  env.Drive(1000, 0, 0);
  info();
  boot("POWERON");
  env.Drive(2000, 0, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(!advised(), ());

  // The box has slept, its clock is 5 % slow then: still the same power-on.
  env.m_clock.Advance(1'000'000);
  env.SleepBox(950'000);
  env.Drive(3000, 0, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(!advised(), ());

  // It has restarted by itself and stays in its place.
  env.RestartBox();
  env.Drive(1000, 0, 0);
  boot("SW");
  env.Drive(2000, 0, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(!advised(), ());

  // Plugged in again while the car drives: the user is told when it stops.
  env.RestartBox();
  env.Drive(1000, 36, 0);
  boot("POWERON");
  env.Drive(3000, 36, 0);
  TEST_EQUAL(count(), 1, ());
  TEST(advised(), ());
  env.Drive(2000, 0, 0);
  TEST_EQUAL(count(), 2, ());
}

UNIT_TEST(NoGps_Engine_AdvisesToCalibrateBoxByItsPowerOns)
{
  Env env(true /* inertial */);
  auto const count = [&env]
  { return std::count(env.m_delegate.m_events.begin(), env.m_delegate.m_events.end(), Event::BoxCalibrationAdvised); };
  // The box tells how many times it was powered on.
  auto const info = [&env](std::string const & id, int powerOns)
  {
    env.m_engine.OnEsp32BleBytes(BoxLine("NGI,1,0.3.5,2026-10-10T11:00,esp32s3,s3zero," + id +
                                         ",4096,2031616,ota_0,VALID,WIFI BLE OTA," + std::to_string(powerOns)));
    env.Drive(2000, 0, 0);
  };

  // The box has got the counting firmware in its place in the car: it has not moved.
  info("C47D", 0);
  TEST_EQUAL(count(), 0, ());
  TEST(!env.m_engine.GetStatus().m_boxCalibrationAdvised, ());

  // It was taken out and plugged in. Its journal tells the same, the user is asked once.
  env.RestartBox();
  env.Drive(1000, 0, 0);
  env.m_engine.OnEsp32BleBytes(BoxLine("NGE,1,1,40,I,BOOT,fw 0.3.5 reset POWERON"));
  info("C47D", 1);
  TEST_EQUAL(count(), 1, ());
  TEST(env.m_engine.GetStatus().m_boxCalibrationAdvised, ());
  // The same power-on after a new connection.
  info("C47D", 1);
  TEST_EQUAL(count(), 1, ());

  // Another box, new for this phone: it is to be calibrated in its car.
  info("42FD", 7);
  TEST_EQUAL(count(), 2, ());
  // The first one is remembered.
  info("C47D", 1);
  info("42FD", 7);
  TEST_EQUAL(count(), 2, ());
  TEST_EQUAL(env.m_storage.Get<std::string>(Storage::kBoxPowerOns, ""), "C47D:1,42FD:7", ());

  // Plugged in again while the car drives: the user is told when it stops.
  env.m_engine.OnEsp32BleBytes(
      BoxLine("NGI,1,0.3.5,2026-10-10T11:00,esp32s3,s3zero,42FD,4096,2031616,ota_0,VALID,WIFI BLE OTA,8"));
  env.Drive(3000, 36, 0);
  TEST_EQUAL(count(), 2, ());
  env.Drive(2000, 0, 0);
  TEST_EQUAL(count(), 3, ());

  // The memory of the box was erased with its calibration: it counts from the start.
  info("42FD", 0);
  TEST_EQUAL(count(), 4, ());
}

UNIT_TEST(NoGps_Engine_OffersFirmwareOfApplication)
{
  auto const count = [](Env const & env, Event event)
  { return std::count(env.m_delegate.m_events.begin(), env.m_delegate.m_events.end(), event); };
  auto const connect = [](Env & env, std::string const & firmware)
  {
    env.m_engine.OnEsp32BleState(BleState::Connected);
    env.m_engine.OnEsp32BleBytes(
        BoxLine("NGI,1," + firmware + ",2026-10-10T07:33,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA"));
  };
  int reads = 0;
  std::vector<Firmware> const firmwares = {{"0.3.3", "esp32s3", "s3zero", [&reads]
  {
    ++reads;
    return std::string(1000, 'x');
  }}};

  {
    // The box works with another firmware than the application comes with: the user is asked once.
    Env env(true /* inertial */);
    env.m_engine.SetFirmwares(firmwares);
    connect(env, "0.3.2");
    // Not at once: the box tells if the car drives in a moment.
    env.m_clock.Advance(Engine::kFirmwareOfferDelayMs - Engine::kTripLogIntervalMs);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 0, ());
    env.m_clock.Advance(Engine::kTripLogIntervalMs * 3);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 1, ());
    auto status = env.m_engine.GetStatus();
    TEST_EQUAL(status.m_boxFirmware, "0.3.2", ());
    TEST_EQUAL(status.m_bundledFirmware, "0.3.3", ());
    TEST_EQUAL(status.m_firmwareUpdate, Esp32Update::State::None, ());
    TEST_EQUAL(reads, 0, ());

    env.m_engine.StartFirmwareUpdate();
    TEST_EQUAL(reads, 1, ());
    status = env.m_engine.GetStatus();
    TEST_EQUAL(status.m_firmwareUpdate, Esp32Update::State::Starting, ());
    TEST_EQUAL(status.m_sourceState, SourceState::Updating, ());
    TEST(status.m_bundledFirmware.empty(), ());
    TEST(env.m_delegate.m_esp32BleSent.back().find(",OTA_BEGIN,1000,") != std::string::npos, ());

    env.m_engine.CancelFirmwareUpdate();
    env.m_clock.Advance(Engine::kTripLogIntervalMs);
    TEST_EQUAL(count(env, Event::FirmwareUpdateFailed), 1, ());
    TEST_EQUAL(env.m_engine.GetStatus().m_firmwareUpdateError, "CANCELED", ());
    // Not asked again about the same firmware.
    env.m_clock.Advance(Engine::kFirmwareOfferDelayMs * 2);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 1, ());
  }
  {
    // The same firmware: nothing to ask about, but it can be sent again.
    Env env(true /* inertial */);
    env.m_engine.SetFirmwares(firmwares);
    connect(env, "0.3.3");
    env.m_clock.Advance(Engine::kFirmwareOfferDelayMs * 2);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 0, ());
    TEST_EQUAL(env.m_engine.GetStatus().m_bundledFirmware, "0.3.3", ());
  }
  {
    // The car drives: the user is asked before the next trip, when the box is connected again.
    Env env(true /* inertial */);
    env.m_engine.SetFirmwares(firmwares);
    connect(env, "0.3.2");
    int64_t seq = 0;
    auto const drive = [&](int64_t ms, int speedKmh)
    {
      for (int64_t t = 0; t < ms; t += 20)
      {
        env.m_clock.Advance(20);
        ++seq;
        env.m_engine.OnEsp32BleBytes(BoxDataLine(seq, 100'000 + seq * 20, 0, speedKmh, kCalibratedFlags));
      }
    };
    drive(1500, 36);
    drive(Engine::kFirmwareOfferDelayMs * 2, 0);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 0, ());
    env.m_engine.OnEsp32BleState(BleState::Searching);
    env.m_clock.Advance(Engine::kTripLogIntervalMs);
    connect(env, "0.3.2");
    drive(Engine::kFirmwareOfferDelayMs + Engine::kTripLogIntervalMs * 2, 0);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 1, ());
  }
  {
    // A box of another kind is not updated.
    Env env(true /* inertial */);
    env.m_engine.SetFirmwares({{"0.3.3", "esp32c3", "c3supermini", [] { return std::string(); }}});
    connect(env, "0.3.2");
    env.m_clock.Advance(Engine::kFirmwareOfferDelayMs * 2);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 0, ());
    auto const status = env.m_engine.GetStatus();
    TEST_EQUAL(status.m_boxFirmware, "0.3.2", ());
    TEST(status.m_bundledFirmware.empty(), ());
  }
  {
    // The application and its firmware are a pair: the box with a newer firmware is offered the older one of the
    // application.
    Env env(true /* inertial */);
    env.m_engine.SetFirmwares(firmwares);
    connect(env, "0.4.0");
    env.m_clock.Advance(Engine::kFirmwareOfferDelayMs * 2);
    TEST_EQUAL(count(env, Event::FirmwareUpdateAvailable), 1, ());
    TEST_EQUAL(env.m_engine.GetStatus().m_bundledFirmware, "0.3.3", ());
  }
}

/// Drives along a street going east at 72 km/h, the car looks aside of the street by |asideDeg(t)| degrees.
/// \param roads the roads instead of the grid of streets with a crossing every 100 m.
/// \returns how far the calculated position is ahead of the car along the street in the end, nothing if the car
/// was taken off the street.
template <class Aside>
std::optional<double> DriveAside(Aside && asideDeg, Roads * roads, double fromEastM, int seconds)
{
  Env env(true /* inertial */, roads);
  MarkCar(env, fromEastM);
  double constexpr kSpeedMps = 20;
  double eastM = fromEastM;
  double asideWas = 0;
  for (int i = 1; i <= seconds * 50; ++i)
  {
    double const aside = asideDeg(i * 0.02);
    env.Drive(20, 72, (aside - asideWas) / 0.02);
    asideWas = aside;
    eastM += kSpeedMps * 0.02 * std::cos(math::DegToRad(aside));
  }
  // The position is aside of the street by a few meters until it is put back.
  if (env.m_delegate.HasEvent(Event::RoadLost) || std::fabs(North(env.Last().m_position)) > 10)
    return {};
  return East(env.Last().m_position) - eastM;
}

UNIT_TEST(NoGps_Engine_WeavingCarIsNotAheadOfItself)
{
  // The car drives a longer way than the road is when it overtakes or goes around holes. The position goes where
  // the car looks at and is put back to the road every second, so only the way along the road counts.
  auto const straight = [](double) { return 0.0; };
  // An overtaking every 10 s: the car turns by 5 degrees within half a second, goes to the next lane for 1.5 s,
  // straightens, and comes back the same way 3 s later. 3 m longer than 2 km of the road.
  auto const overtaking = [](double t)
  {
    auto const change = [](double s)
    {
      if (s < 0 || s >= 2.5)
        return 0.0;
      return 5.0 * std::min({s / 0.5, 1.0, (2.5 - s) / 0.5});
    };
    double const s = std::fmod(t, 10.0);
    return change(s) - change(s - 5.5);
  };
  // A snake: 1.8 m to each side every 80 m, 10 m longer than 2 km of the road. A wild one: 2.5 m to each side
  // every 60 m, 34 m longer.
  auto const snake = [](double t) { return 8 * std::sin(2 * math::pi * t / 4); };
  auto const wildSnake = [](double t) { return 15 * std::sin(2 * math::pi * t / 3); };

  // 2 km of a road without crossings, and 900 m of the street with them.
  PieceRoads road({{-3000, 0, 3000, 0}}, {});
  for (Roads * roads : {static_cast<Roads *>(&road), static_cast<Roads *>(nullptr)})
  {
    double const from = roads ? -2500 : -450;
    int const seconds = roads ? 100 : 45;
    auto const base = DriveAside(straight, roads, from, seconds);
    auto const overtakes = DriveAside(overtaking, roads, from, seconds);
    auto const snakes = DriveAside(snake, roads, from, seconds);
    auto const wildSnakes = DriveAside(wildSnake, roads, from, seconds);
    TEST(base && overtakes && snakes && wildSnakes, (roads != nullptr));
    LOG(LWARNING, (roads ? "A road without crossings, 2 km." : "A street with a crossing every 100 m, 900 m.",
                   "Ahead of the car: straight", *base, "m, overtaking", *overtakes, "m, a snake", *snakes,
                   "m, a wild snake", *wildSnakes, "m"));
    TEST_LESS(std::fabs(*overtakes - *base), 1, ());
    TEST_LESS(std::fabs(*snakes - *base), 1, ());
    TEST_LESS(std::fabs(*wildSnakes - *base), 1, ());
  }
}

UNIT_TEST(NoGps_Engine_GpsIsBackAfterRoadIsLost)
{
  Env env(true /* inertial */);
  MarkCar(env, -400);
  // The heading goes wrong slowly, slower than a turn: the car leaves the road, the calculation stops.
  env.Drive(12'000, 36, -5);
  env.Drive(5000, 36, 0);
  TEST(env.m_delegate.HasEvent(Event::RoadLost), ());
  TEST(env.m_engine.IsManualMode(), ());
  // GPS works again, far from where the calculation has stopped: there is nothing to compare it with, it
  // is trusted after it has worked for long enough.
  for (int i = 0; i <= GpsReturnDetector::kMinStreakMs / 1000 + 1; ++i)
  {
    env.Drive(1000, 36, 0);
    env.m_engine.OnFix(GpsAt(env.m_clock, 10 * i, 300, 90.0, 10.0));
  }
  TEST(!env.m_engine.IsManualMode(), ());
  TEST(env.m_delegate.HasEvent(Event::GpsBack), ());
  TEST_EQUAL(env.Last().m_provider, Provider::Gps, ());
}

UNIT_TEST(NoGps_Engine_TripLineHasPlatformPositions)
{
  Env env(true /* inertial */);
  TEST(env.m_engine.GetTripLine().find(" gps=") == std::string::npos, ());

  env.m_clock.Advance(1000);
  env.m_engine.OnFix(GpsAt(env.m_clock, 100, 0, 90.0, 10.0, 4));
  env.m_clock.Advance(250);
  char expected[128];
  std::snprintf(expected, sizeof(expected), " gps=%.6f,%.6f/4.0/36.0/90.0/0.25/- ", At(100, 0).m_lat, At(100, 0).m_lon);
  std::string line = env.m_engine.GetTripLine();
  TEST(line.find(expected) != std::string::npos, (line));

  // The position GPS is ignored with, e.g. to test the inertial navigation, is written too: the shown position is
  // compared with it. This one has no speed and no bearing.
  env.m_engine.SetGpsDisabled(true);
  env.m_clock.Advance(1000);
  env.m_engine.OnFix(GpsAt(env.m_clock, 200, 0));
  std::snprintf(expected, sizeof(expected), " gps=%.6f,%.6f/5.0/-/-/0.00/- ", At(200, 0).m_lat, At(200, 0).m_lon);
  line = env.m_engine.GetTripLine();
  TEST(line.find(expected) != std::string::npos, (line));

  // The satellites mixed with Wi-Fi and cell towers, the only position of iOS, are written apart.
  TEST(line.find(" fused=") == std::string::npos, (line));
  auto fused = GpsAt(env.m_clock, 300, 0, 90.0, 10.0, 8);
  fused.m_provider = Provider::Fused;
  env.m_engine.OnFix(fused);
  std::snprintf(expected, sizeof(expected), " fused=%.6f,%.6f/8.0/36.0/90.0/0.00/- ", At(300, 0).m_lat,
                At(300, 0).m_lon);
  line = env.m_engine.GetTripLine();
  TEST(line.find(expected) != std::string::npos, (line));
  TEST(line.find(" gps=") != std::string::npos, (line));

  // An old position tells nothing about the car now.
  env.m_clock.Advance(Engine::kGpsStatusMaxAgeMs + 1000);
  line = env.m_engine.GetTripLine();
  TEST(line.find(" gps=") == std::string::npos && line.find(" fused=") == std::string::npos, (line));
}

UNIT_TEST(NoGps_Engine_TripLineTellsWhyCarIsNotOnRoad)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  env.Drive(3000, 9, 0);
  std::string line = env.m_engine.GetTripLine();
  TEST(line.find(" rdiff=0 snapwhy=ROAD ") != std::string::npos, (line));
  TEST(line.find(" speed=9 sage=0 ") != std::string::npos, (line));
  TEST(line.find(" turning=0 odo=7 offm=0 offv=0 cand=- ") != std::string::npos, (line));

  // The car turns right by 40 degrees: the street goes another way than it looks.
  env.Drive(1000, 9, 40);
  TEST(env.m_engine.GetTripLine().find(" turning=1 ") != std::string::npos, (env.m_engine.GetTripLine()));
  env.Drive(2500, 9, 0);
  line = env.m_engine.GetTripLine();
  TEST(line.find(" rdiff=40 snapwhy=ANGLE ") != std::string::npos, (line));
  TEST(line.find(" turning=0 odo=16 offm=4 offv=9 cand=- ") != std::string::npos, (line));

  // And to the south: no street goes that way.
  env.Drive(1000, 9, 50);
  env.Drive(1500, 9, 0);
  line = env.m_engine.GetTripLine();
  TEST(line.find(" rdiff=- snapwhy=NOROAD ") != std::string::npos, (line));
}

UNIT_TEST(NoGps_Engine_PauseResumesByItself)
{
  Env env(true /* inertial */);
  MarkCar(env, 50);
  env.Drive(2000, 36, 0);
  env.m_engine.TogglePause();
  TEST(env.m_engine.GetStatus().m_paused, ());
  // The car maneuvers: the speed is positive, but the position stays. The positions are sent 5 times a second, the
  // first one after the pause is where the car has stopped.
  env.Drive(2 * InertialNavigator::kOutputIntervalMs, 10, 0);
  double const pausedAtM = East(env.Last().m_position);
  env.Drive(2000, 10, 0);
  TestAlmostEqualAbs(East(env.Last().m_position), pausedAtM, 0.01);
  // Nobody drives so fast backwards: the pause is over.
  env.Drive(3500, 20, 0);
  TEST(!env.m_engine.GetStatus().m_paused, ());
  env.Drive(1000, 20, 0);
  TEST_GREATER(East(env.Last().m_position), pausedAtM, ());
}

/// The car marked on the street going east leaves it to the right at a walking speed and drives 24 m into a yard.
void DriveIntoYard(Env & env)
{
  MarkCar(env, 0);
  env.Drive(2000, 9, 0);
  env.Drive(1000, 9, 90);
  env.Drive(9000, 9, 0);
}

UNIT_TEST(NoGps_Engine_ManeuverOffRoadsPausesByItself)
{
  // A street going east, and a street 40 m to the south of it going the same way.
  PieceRoads roads({{-300, 0, 300, 0}, {-300, -40, 600, -40}}, {});
  for (bool navigating : {true, false})
  {
    Env env(true /* inertial */, &roads);
    env.m_map.m_navigating = navigating;
    DriveIntoYard(env);
    auto status = env.m_engine.GetStatus();
    TEST(status.m_paused, (navigating));
    TEST(status.m_pausedByItself, (navigating));
    TEST(!env.m_delegate.HasEvent(Event::RoadLost), (navigating));
    TEST(env.m_engine.GetTripLine().find(" self=1 car=") != std::string::npos, (env.m_engine.GetTripLine()));
    if (navigating)
    {
      // The mark waits on the street of the route, the car is shown apart in the yard.
      TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);
      TEST(env.m_map.m_offRoadCar, ());
      TestAlmostEqualAbs(North(*env.m_map.m_offRoadCar), -24, 3);
      TestAlmostEqualAbs(East(*env.m_map.m_offRoadCar), 7, 3);
    }
    else
    {
      // The mark follows the car, there is nothing to show apart.
      TestAlmostEqualAbs(North(env.Last().m_position), -24, 3);
      TestAlmostEqualAbs(East(env.Last().m_position), 7, 3);
      TEST(!env.m_map.m_offRoadCar, ());
    }
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 180, 1);

    // The car turns left and drives east, the other street is 15 m to the right of it. It is not the street of
    // the car at once.
    env.Drive(1000, 9, -90);
    env.Drive(3000, 9, 0);
    TEST(env.m_engine.GetStatus().m_pausedByItself, (navigating));
    // The car has driven along it for 15 m: it is on it, looking along it.
    env.Drive(6000, 9, 0);
    status = env.m_engine.GetStatus();
    TEST(!status.m_paused, (navigating));
    TEST(!status.m_pausedByItself, (navigating));
    TEST(!env.m_map.m_offRoadCar, (navigating));
    TestAlmostEqualAbs(North(env.Last().m_position), -40, 0.5);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.5);
    double const east = East(env.Last().m_position);
    TEST_GREATER(east, 20, ());
    env.Drive(4000, 9, 0);
    TestAlmostEqualAbs(North(env.Last().m_position), -40, 0.5);
    TestAlmostEqualAbs(East(env.Last().m_position), east + 10, 3);
    TEST(!env.m_delegate.HasEvent(Event::RoadLost), (navigating));
    TEST(env.m_engine.GetTripLine().find(" self=1") == std::string::npos, (env.m_engine.GetTripLine()));
  }
}

UNIT_TEST(NoGps_Engine_PauseSetByItselfWaitsForUser)
{
  // The street ends 20 m to the east of the car, there are no other roads.
  PieceRoads roads({{-300, 0, 20, 0}}, {});
  Env env(true /* inertial */, &roads);
  env.m_map.m_navigating = true;
  DriveIntoYard(env);
  TEST(env.m_engine.GetStatus().m_pausedByItself, ());
  double const markEast = East(env.Last().m_position);

  // The car turns east and drives fast where the map has no roads: nobody maneuvers so, but there is no road
  // to put the car on. The mark waits, the user is told.
  env.Drive(1000, 9, -90);
  env.Drive(8000, 36, 0);
  auto status = env.m_engine.GetStatus();
  TEST(status.m_paused, ());
  TEST(status.m_pausedByItself, ());
  TEST(env.m_delegate.HasEvent(Event::RoadLost), ());
  TestAlmostEqualAbs(East(env.Last().m_position), markEast, 0.01);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);
  TEST(env.m_map.m_offRoadCar, ());
  TEST_GREATER(East(*env.m_map.m_offRoadCar), 80, ());

  // The user resumes the movement, but there is no road next to the car to go on from: the pause stays, the car
  // is where it was, and the user is told to mark it on a road.
  env.Drive(1000, 0, 0);
  auto const car = *env.m_map.m_offRoadCar;
  TEST(!env.m_delegate.HasEvent(Event::MarkNoRoad), ());
  env.m_engine.TogglePause();
  status = env.m_engine.GetStatus();
  TEST(status.m_paused, ());
  TEST(status.m_pausedByItself, ());
  TEST(env.m_delegate.HasEvent(Event::MarkNoRoad), ());
  env.Drive(1000, 0, 0);
  TEST(env.m_map.m_offRoadCar, ());
  TestAlmostEqualAbs(East(*env.m_map.m_offRoadCar), East(car), 0.5);
  TestAlmostEqualAbs(North(*env.m_map.m_offRoadCar), North(car), 0.5);
  TestAlmostEqualAbs(East(env.Last().m_position), markEast, 0.5);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);

  // The mark of the user ends the pause.
  TEST(env.m_engine.PlaceMarkByTap(At(-100, 2)), ());
  status = env.m_engine.GetStatus();
  TEST(!status.m_paused, ());
  TEST(!status.m_pausedByItself, ());
  TEST(!env.m_map.m_offRoadCar, ());
}

UNIT_TEST(NoGps_Engine_PauseSetByItselfIsEndedByUserNextToRoad)
{
  // A street going east, and a street 40 m to the south of it going the same way.
  PieceRoads roads({{-300, 0, 300, 0}, {-300, -40, 600, -40}}, {});
  for (bool navigating : {true, false})
  {
    Env env(true /* inertial */, &roads);
    env.m_map.m_navigating = navigating;
    DriveIntoYard(env);
    TEST(env.m_engine.GetStatus().m_pausedByItself, (navigating));

    // The car turns left 16 m from the other street and stops. The user resumes the movement: the car goes on from
    // that street, not from where it has left the first one.
    env.Drive(1000, 9, -90);
    env.Drive(1000, 0, 0);
    env.m_engine.TogglePause();
    auto const status = env.m_engine.GetStatus();
    TEST(!status.m_paused, (navigating));
    TEST(!status.m_pausedByItself, (navigating));
    TEST(!env.m_delegate.HasEvent(Event::MarkNoRoad), (navigating));
    env.Drive(1000, 0, 0);
    TestAlmostEqualAbs(North(env.Last().m_position), -40, 0.5);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.5);
    TEST(!env.m_map.m_offRoadCar, (navigating));
  }
}

UNIT_TEST(NoGps_Engine_MarkOfUserEndsPauseSetByItself)
{
  PieceRoads roads({{-300, 0, 300, 0}}, {});
  Env env(true /* inertial */, &roads);
  env.m_map.m_navigating = true;
  DriveIntoYard(env);
  TEST(env.m_engine.GetStatus().m_pausedByItself, ());
  TEST(env.m_map.m_offRoadCar, ());

  // The car is where the user tells, not where it has maneuvered to by the calculation.
  TEST(env.m_engine.PlaceMarkByTap(At(100, 2)), ());
  auto const status = env.m_engine.GetStatus();
  TEST(!status.m_paused, ());
  TEST(!status.m_pausedByItself, ());
  TEST(!env.m_map.m_offRoadCar, ());
  env.Drive(2000, 0, 0);
  TestAlmostEqualAbs(East(env.Last().m_position), 100, 0.5);
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);

  // The pause of the user is not the one set by itself: the car is not followed apart.
  env.Drive(2000, 9, 0);
  env.m_engine.TogglePause();
  env.Drive(1000, 9, 90);
  env.Drive(9000, 9, 0);
  TEST(env.m_engine.GetStatus().m_paused, ());
  TEST(!env.m_engine.GetStatus().m_pausedByItself, ());
  TEST(!env.m_map.m_offRoadCar, ());
}

UNIT_TEST(NoGps_Engine_FastCarComesBackToRoadFarAside)
{
  // A street going east that ends 20 m ahead of the car, and a street 55 m to the south of it going the same way.
  PieceRoads roads({{-300, 0, 20, 0}, {-300, -55, 900, -55}}, {});
  Env env(true /* inertial */, &roads);
  env.m_map.m_navigating = true;
  DriveIntoYard(env);
  TEST(env.m_engine.GetStatus().m_pausedByItself, ());
  // The car turns east and drives 30 m from the other street, too far to be put on it while it maneuvers.
  env.Drive(1000, 9, -90);
  env.Drive(8000, 9, 0);
  TEST(env.m_engine.GetStatus().m_pausedByItself, ());
  // It drives fast for 3 s: nobody maneuvers so, it is on the street going its way.
  env.Drive(3500, 36, 0);
  TEST(!env.m_engine.GetStatus().m_paused, ());
  TEST(!env.m_map.m_offRoadCar, ());
  env.Drive(1000, 36, 0);
  TestAlmostEqualAbs(North(env.Last().m_position), -55, 0.5);
  TestAlmostEqualAbs(*env.Last().m_bearingDeg, 90, 0.5);
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_ManeuveringCarIsNotThrownToRoadAside)
{
  // A street going east and a street going south from it at 30 m. The map doesn't tell that they cross: the turn
  // of the car is not moved to the crossing.
  PieceRoads roads({{-300, 0, 600, 0}, {30, 0, 30, -600}}, {});
  for (int speedKmh : {9, 36})
  {
    Env env(true /* inertial */, &roads);
    MarkCar(env, 0);
    // The car turns right 12 m before the other street or more. The calculated position is ahead of what the test
    // counts by the lag of the car speed.
    double const speedMps = speedKmh / 3.6;
    env.Drive(std::llround((15 - speedMps * 0.64) / speedMps * 1000), speedKmh, 0);
    env.Drive(1000, speedKmh, 90);
    env.Drive(3000, speedKmh, 0);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 180, 3);
    if (speedKmh == 9)
    {
      // A maneuvering car is not on that street yet: next to a yard such a street is behind the houses.
      TEST_GREATER(East(env.Last().m_position), 14, ());
      TEST_LESS(East(env.Last().m_position), 19.5, ());
      TEST(!env.m_engine.GetStatus().m_paused, ());
      // It has driven along the street for 15 m: it is put on it, and was not paused meanwhile.
      env.Drive(2000, speedKmh, 0);
      TEST(!env.m_engine.GetStatus().m_paused, ());
      env.Drive(4000, speedKmh, 0);
    }
    // A car driving along a road is thrown to it at once.
    TestAlmostEqualAbs(East(env.Last().m_position), 30, 0.5);
    TestAlmostEqualAbs(*env.Last().m_bearingDeg, 180, 1);
    TEST(!env.m_engine.GetStatus().m_paused, (speedKmh));
    TEST(!env.m_delegate.HasEvent(Event::RoadLost), (speedKmh));
  }
}

/// A car with the sensor box: it speeds up, brakes and turns as the test tells, backwards too. The box reports the
/// speed of the car 1.2 s late and without a sign.
class BoxCar
{
public:
  /// \param accelerometer the box has one, it looks forward with its first horizontal axis.
  BoxCar(Env & env, double eastM, bool accelerometer)
    : m_env(env)
    , m_accelerometer(accelerometer)
    , m_east(eastM)
    , m_speeds(60, 0.0)
  {}

  /// \param accelerationMps2 along the car, negative to brake or to speed up backwards.
  void Drive(double seconds, double accelerationMps2, double yawRateDegS)
  {
    for (int i = 0; i < std::lround(seconds * 50); ++i)
    {
      m_speed += accelerationMps2 * 0.02;
      m_headingDeg += yawRateDegS * 0.02;
      m_yawMdeg += yawRateDegS * 20;
      m_east += m_speed * 0.02 * std::sin(math::DegToRad(m_headingDeg));
      m_north += m_speed * 0.02 * std::cos(math::DegToRad(m_headingDeg));
      m_speeds.push_back(std::fabs(m_speed));
      int const reportedKmh = static_cast<int>(std::lround(m_speeds.front() * 3.6));
      m_speeds.pop_front();

      m_env.m_clock.Advance(20);
      m_boxTimeMs += 20;
      char body[160];
      int const length = std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,%lld,0,0,%d,0,%X",
                                       static_cast<long long>(++m_seq), static_cast<long long>(m_boxTimeMs),
                                       static_cast<long long>(std::llround(m_yawMdeg)), reportedKmh, kCalibratedFlags);
      // A little of the gravity is left in the other axes.
      if (m_accelerometer)
        std::snprintf(body + length, sizeof(body) - length, ",%d,50,-60,100",
                      static_cast<int>(std::lround((accelerationMps2 + m_slopeMps2) * 1000)));
      m_env.m_engine.OnEsp32BleBytes(BoxLine(body));
    }
  }

  /// The car speeds up and brakes four times, 900 m along its way, and stands: the box learns where its
  /// accelerometer looks.
  void SpeedUpAndBrake()
  {
    for (int i = 0; i < 4; ++i)
    {
      Drive(10, 1.5, 0);
      Drive(5, 0, 0);
      Drive(10, -1.5, 0);
      Drive(5, 0, 0);
    }
  }

  double East() const { return m_east; }
  double North() const { return m_north; }

  // The gravity the accelerometer feels along the car on a slope.
  double m_slopeMps2 = 0;

private:
  Env & m_env;
  bool const m_accelerometer;
  double m_east;
  double m_north = 0;
  double m_headingDeg = 90;
  // Negative while the car drives backwards.
  double m_speed = 0;
  // The speeds of the car of the last 1.2 s.
  std::deque<double> m_speeds;
  int64_t m_seq = 1000;
  int64_t m_boxTimeMs = 200'000;
  double m_yawMdeg = 0;
};

UNIT_TEST(NoGps_Engine_AccelerometerTellsCarInYardDrivesBack)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  for (bool accelerometer : {true, false})
  {
    Env env(true /* inertial */, &roads);
    MarkCar(env, -450);
    BoxCar car(env, -450, accelerometer);
    car.SpeedUpAndBrake();
    TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 5);

    // It turns right into a yard at a walking speed, drives 20 m and stops there.
    car.Drive(2.5, 1, 0);
    car.Drive(1, 0, 90);
    car.Drive(8, 0, 0);
    car.Drive(2.5, -1, 0);
    car.Drive(4, 0, 0);
    TEST(env.m_engine.GetStatus().m_pausedByItself, (accelerometer));
    TestAlmostEqualAbs(North(env.Last().m_position), car.North(), 4);
    double const stoppedNorth = North(env.Last().m_position);
    TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));

    // It drives 12 m backwards. The speed of the car tells the same as if it drove forward.
    car.Drive(2, -1, 0);
    car.Drive(2, 0, 0);
    TEST_EQUAL(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, accelerometer,
               (env.m_engine.GetTripLine()));
    car.Drive(2, 0, 0);
    car.Drive(2, 1, 0);
    car.Drive(3, 0, 0);
    TEST(env.m_engine.GetStatus().m_pausedByItself, (accelerometer));
    if (accelerometer)
    {
      TestAlmostEqualAbs(North(env.Last().m_position), stoppedNorth + 12, 3);
      TestAlmostEqualAbs(North(env.Last().m_position), car.North(), 4);
    }
    else
    {
      TestAlmostEqualAbs(North(env.Last().m_position), stoppedNorth - 12, 3);
    }

    // And forward again, 12 m more than it has driven back.
    car.Drive(2, 1, 0);
    car.Drive(10, 0, 0);
    car.Drive(2, -1, 0);
    car.Drive(3, 0, 0);
    TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
    if (accelerometer)
      TestAlmostEqualAbs(North(env.Last().m_position), car.North(), 4);
  }
}

UNIT_TEST(NoGps_Engine_TurnStartedBeforeCarReportsSpeedIsNotLost)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, 0);
  BoxCar car(env, 0, false /* accelerometer */);
  car.Drive(3, 0, 0);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 90), 0, 1);
  // The car starts and turns right at once, by 90 degrees in 4.5 s. It reports that it moves 1.3 s later: it has
  // turned by 26 degrees by then.
  car.Drive(4.5, 1, 20);
  car.Drive(1.5, 0, 0);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 180), 0, 2);
}

UNIT_TEST(NoGps_Engine_AccelerometerTellsCarOnRoadDrivesBack)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  for (bool accelerometer : {true, false})
  {
    Env env(true /* inertial */, &roads);
    MarkCar(env, -450);
    BoxCar car(env, -450, accelerometer);
    car.SpeedUpAndBrake();
    TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 5);
    double const stoppedEast = East(env.Last().m_position);

    // It drives 12 m backwards along the street. The speed of the car tells the same as if it drove forward.
    car.Drive(2, -1, 0);
    car.Drive(2, 0, 0);
    TEST_EQUAL(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, accelerometer,
               (env.m_engine.GetTripLine()));
    car.Drive(2, 0, 0);
    car.Drive(2, 1, 0);
    car.Drive(3, 0, 0);
    TestAlmostEqualAbs(East(env.Last().m_position), stoppedEast + (accelerometer ? -12 : 12), 3);
    TestAlmostEqualAbs(North(env.Last().m_position), 0, 1);
    // The car looks where it did.
    TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 90), 0, 3);
    TEST(!env.m_engine.GetStatus().m_paused, (accelerometer));

    // And forward again.
    car.Drive(2, 1, 0);
    car.Drive(10, 0, 0);
    car.Drive(2, -1, 0);
    car.Drive(3, 0, 0);
    TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
    if (accelerometer)
      TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 4);
  }
}

UNIT_TEST(NoGps_Engine_CarBackingIntoSideStreetIsFollowed)
{
  // A street crosses the street going east at 0 m.
  PieceRoads roads({{-2000, 0, 3000, 0}, {0, -500, 0, 500}}, {{0, 0}});
  Env env(true /* inertial */, &roads);
  MarkCar(env, -890);
  BoxCar car(env, -890, true /* accelerometer */);
  // The car stops 10 m after the crossing, the user marks it there.
  car.SpeedUpAndBrake();
  TestAlmostEqualAbs(car.East(), 10, 0.5);
  TEST(env.m_engine.PlaceMarkByTap(At(car.East(), 3)), ());

  // It backs into the side street to the south: 2 m along its street, a quarter of a circle of 8 m, and 2 m along
  // the side street, looking north in the end.
  car.Drive(2, -1, 0);
  car.Drive(6.3, 0, -14.3);
  car.Drive(2, 1, 0);
  car.Drive(3, 0, 0);
  TestAlmostEqualAbs(car.East(), 0, 0.5);
  TestAlmostEqualAbs(car.North(), -10, 0.5);
  TestAlmostEqualAbs(East(env.Last().m_position), 0, 1);
  TestAlmostEqualAbs(North(env.Last().m_position), car.North(), 4);
  TestAlmostEqualAbs(AngleDiff(*env.Last().m_bearingDeg, 0), 0, 5);
  // A car rotating that way driving forward turns left at the crossing: this one is not moved as if it did.
  TEST(env.m_engine.GetTripLine().find(" turn=NaN") != std::string::npos, (env.m_engine.GetTripLine()));

  // And drives forward across the crossing.
  car.Drive(2, 1, 0);
  car.Drive(8, 0, 0);
  car.Drive(2, -1, 0);
  car.Drive(3, 0, 0);
  TestAlmostEqualAbs(car.North(), 10, 0.5);
  TestAlmostEqualAbs(East(env.Last().m_position), 0, 1);
  TestAlmostEqualAbs(North(env.Last().m_position), car.North(), 4);
  TEST(!env.m_engine.GetStatus().m_paused, ());
  TEST(!env.m_delegate.HasEvent(Event::RoadLost), ());
}

UNIT_TEST(NoGps_Engine_CarDrivingFastDoesNotDriveBack)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, -450);
  BoxCar car(env, -450, true /* accelerometer */);
  car.SpeedUpAndBrake();
  double const stoppedEast = East(env.Last().m_position);

  // The street goes down from where the car stood: the car starts forward, the accelerometer feels a start backward.
  car.m_slopeMps2 = -2.5;
  car.Drive(3, 1, 0);
  TEST(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, (env.m_engine.GetTripLine()));
  TEST_LESS(East(env.Last().m_position), stoppedEast, ());

  // Nobody drives 15 km/h backwards: the car is where it would be if it was known to drive forward.
  car.m_slopeMps2 = 0;
  car.Drive(3, 1, 0);
  car.Drive(5, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
  TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 5);
}

UNIT_TEST(NoGps_Engine_CarStartingWithJerkBackDrivesForward)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, -450);
  BoxCar car(env, -450, true /* accelerometer */);
  car.SpeedUpAndBrake();
  double const stoppedEast = East(env.Last().m_position);

  // The car creeps forward, and its first jerk is felt as a start backward.
  car.m_slopeMps2 = -2.5;
  car.Drive(0.3, 1, 0);
  car.m_slopeMps2 = 0;
  car.Drive(1.6, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, (env.m_engine.GetTripLine()));

  // It speeds up to 12 km/h, slower than a car that is known to drive forward by its speed. The accelerometer feels
  // the speed the car reports, growing forward.
  car.Drive(3, 1, 0);
  car.Drive(2, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
  TEST_GREATER(East(env.Last().m_position), stoppedEast + 10, ());
  TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 3);
}

UNIT_TEST(NoGps_Engine_CarDrivingBackAfterShortStopIsFollowed)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, -450);
  BoxCar car(env, -450, true /* accelerometer */);
  car.SpeedUpAndBrake();

  // The car parks: 6 m forward, a stop of a second, which is too short to feel what the accelerometer feels
  // standing, and 8 m backward.
  car.Drive(2, 1, 0);
  car.Drive(1, 0, 0);
  car.Drive(2, -1, 0);
  car.Drive(1, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
  car.Drive(2, -1, 0);
  car.Drive(2, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, (env.m_engine.GetTripLine()));
  car.Drive(2, 1, 0);
  car.Drive(3, 0, 0);
  TestAlmostEqualAbs(East(env.Last().m_position), car.East(), 3);
}

UNIT_TEST(NoGps_Engine_AccelerometerIsNotLearnedFromManeuvers)
{
  PieceRoads roads({{-2000, 0, 3000, 0}}, {});
  Env env(true /* inertial */, &roads);
  MarkCar(env, -450);
  BoxCar car(env, -450, true /* accelerometer */);
  car.SpeedUpAndBrake();

  // The car backs down a slope that begins where it stood: the accelerometer feels a start forward.
  car.m_slopeMps2 = 3;
  car.Drive(2, -1, 0);
  TEST(env.m_engine.GetTripLine().find(" back=0") != std::string::npos, (env.m_engine.GetTripLine()));
  // It speeds up and brakes for a minute at 4-11 km/h, driving backwards: its speed grows while the accelerometer
  // feels it braking.
  car.Drive(1, -1, 0);
  for (int i = 0; i < 15; ++i)
  {
    car.Drive(2, 1, 0);
    car.Drive(2, -1, 0);
  }
  car.Drive(3, 1, 0);
  car.Drive(5, 0, 0);

  // The accelerometer still knows where the car looks: the next start backward is told.
  car.Drive(2, -1, 0);
  car.Drive(2, 0, 0);
  TEST(env.m_engine.GetTripLine().find(" back=1") != std::string::npos, (env.m_engine.GetTripLine()));
}

UNIT_TEST(NoGps_Engine_ManualModeWhenGpsIsLostWhileNavigating)
{
  Env env(false /* inertial */);
  env.m_map.m_navigating = true;
  env.DriveGps(10, 0, 10);
  TEST_EQUAL(env.Last().m_provider, Provider::Gps, ());
  TEST(!env.m_engine.IsManualMode(), ());

  // GPS is jammed.
  env.m_clock.Advance(Engine::kGpsLostMs + 1500);
  TEST(env.m_engine.IsManualMode(), ());
  TEST(env.m_delegate.HasEvent(Event::GpsLost), ());
  // The car is where it was shown, on the road.
  TEST_EQUAL(env.Last().m_provider, Provider::Manual, ());
  TestAlmostEqualAbs(East(env.Last().m_position), 90, 1);

  // GPS works again for long enough near the car.
  for (int i = 0; i < 16; ++i)
  {
    env.m_clock.Advance(1000);
    env.m_engine.OnFix(GpsAt(env.m_clock, 90, 0, {}, 0.0));
  }
  TEST(!env.m_engine.IsManualMode(), ());
  TEST(env.m_delegate.HasEvent(Event::GpsBack), ());
  TEST_EQUAL(env.Last().m_provider, Provider::Gps, ());
}

UNIT_TEST(NoGps_Engine_ManualModeWhenGpsIsSpoofed)
{
  Env env(false /* inertial */);
  env.m_map.m_navigating = true;
  Fix network = GpsAt(env.m_clock, 0, 0, {}, {}, 100);
  network.m_provider = Provider::Network;
  env.m_engine.OnFix(network);
  env.DriveGps(3, 0, 10);
  TEST(!env.m_engine.IsManualMode(), ());

  // GPS is on another continent.
  env.m_clock.Advance(1000);
  Fix spoofed = GpsAt(env.m_clock, 0, 0, 90.0, 10.0);
  spoofed.m_position = {-12.0453, -77.0569};
  env.m_engine.OnFix(spoofed);
  TEST(env.m_engine.GetStatus().m_gpsSpoofed, ());
  TEST(env.m_delegate.HasEvent(Event::GpsSpoofed), ());
  TEST(env.m_engine.IsManualMode(), ());
  TEST_EQUAL(env.Last().m_provider, Provider::Manual, ());
}

UNIT_TEST(NoGps_Engine_ManualModeWhenGpsLeavesRoads)
{
  Env env(false /* inertial */);
  env.m_map.m_navigating = true;
  env.DriveGps(3, 0, 10);
  // A single position between the streets is noise.
  for (int i = 0; i + 1 < Engine::kOffRoadGpsCount; ++i)
  {
    env.m_clock.Advance(1000);
    env.m_engine.OnFix(GpsAt(env.m_clock, 50, 50, 90.0, 10.0));
  }
  TEST(!env.m_engine.IsManualMode(), ());
  env.m_clock.Advance(1000);
  env.m_engine.OnFix(GpsAt(env.m_clock, 50, 50, 90.0, 10.0));
  TEST(env.m_engine.IsManualMode(), ());
  // The car stays on the road where GPS was trusted the last time.
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 0.5);
}

UNIT_TEST(NoGps_Engine_DisabledGpsIsIgnored)
{
  Env env(false /* inertial */);
  env.m_engine.SetGpsDisabled(true);
  env.DriveGps(3, 0, 10);
  TEST(env.m_delegate.m_positions.empty(), ());
  TEST(!env.m_engine.GetStatus().m_workingGpsAccuracyM, ());
}

UNIT_TEST(NoGps_Engine_InertialContinuesWhenGpsIsLost)
{
  Env env(true /* inertial */);
  // GPS gives the position and the heading while the box tells the speed.
  for (int i = 0; i < 5; ++i)
  {
    env.Drive(1000, 36, 0);
    env.m_engine.OnFix(GpsAt(env.m_clock, 10 * i, 0, 90.0, 10.0));
  }
  TEST_EQUAL(env.Last().m_provider, Provider::Gps, ());
  // GPS is lost: the manual mode is turned on by itself and the car is followed by the inertial navigation.
  env.Drive(Engine::kGpsLostMs + 2000, 36, 0);
  TEST(env.m_engine.IsManualMode(), ());
  TEST_EQUAL(env.Last().m_provider, Provider::Inertial, ());
  TEST_EQUAL(env.m_engine.GetStatus().m_source, PositionSource::Inertial, ());
  TEST_GREATER(East(env.Last().m_position), 90, ());
  TestAlmostEqualAbs(North(env.Last().m_position), 0, 1);
}
// The car speeds up to 54 km/h in 10 s, drives 5 s, brakes in 10 s and stands 5 s, again and again, going east
// along the street. The box reports the car speed 1.2 s late. Returns how far the calculated position was from the
// car at most 1.5-3 s after the car started to speed up or to brake, when the box has been learned.
double DriveWithLateSpeed(bool accelerometer)
{
  double constexpr kLagSec = 1.2;
  double constexpr kCycleSec = 30;
  auto const acceleration = [](double timeSec)
  {
    double const t = std::fmod(timeSec, kCycleSec);
    return t < 10 ? 1.5 : t >= 15 && t < 25 ? -1.5 : 0.0;
  };

  Env env(true /* inertial */);
  MarkCar(env, -450);
  int64_t seq = 1000;
  int64_t boxTimeMs = 200'000;
  double speed = 0;
  double east = -450;
  // The speeds of the car of the last seconds.
  std::deque<double> speeds(static_cast<size_t>(kLagSec / 0.02), 0.0);
  double maxError = 0;
  for (int step = 0; step < 4 * kCycleSec * 50; ++step)
  {
    double const t = step * 0.02;
    double const a = acceleration(t);
    east += speed * 0.02 + a * 0.02 * 0.02 / 2;
    speed = std::max(0.0, speed + a * 0.02);
    speeds.push_back(speed);
    int const reportedKmh = static_cast<int>(std::lround(speeds.front() * 3.6));
    speeds.pop_front();

    env.m_clock.Advance(20);
    boxTimeMs += 20;
    char body[160];
    if (accelerometer)
    {
      // The box looks forward with its first horizontal axis, a little of the gravity is left in the others.
      std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,0,0,0,%d,0,%X,%d,50,-60,100", static_cast<long long>(++seq),
                    static_cast<long long>(boxTimeMs), reportedKmh, kCalibratedFlags, static_cast<int>(a * 1000));
    }
    else
    {
      std::snprintf(body, sizeof(body), "NGD,1,%lld,%lld,0,0,0,%d,0,%X", static_cast<long long>(++seq),
                    static_cast<long long>(boxTimeMs), reportedKmh, kCalibratedFlags);
    }
    size_t const positions = env.m_delegate.m_positions.size();
    env.m_engine.OnEsp32BleBytes(BoxLine(body));
    // The third and the fourth time the car speeds up and brakes.
    double const inCycle = std::fmod(t, kCycleSec);
    bool const changing = (inCycle >= 1.5 && inCycle < 3) || (inCycle >= 16.5 && inCycle < 18);
    if (t >= 2 * kCycleSec && changing && env.m_delegate.m_positions.size() > positions)
      maxError = std::max(maxError, std::fabs(East(env.Last().m_position) - east));
  }
  TEST_EQUAL(env.Last().m_provider, Provider::Inertial, ());
  TEST_EQUAL(env.m_engine.GetStatus().m_source, PositionSource::Inertial, ());
  return maxError;
}

UNIT_TEST(NoGps_Engine_BoxAccelerometerTellsSpeedBeforeCar)
{
  // The car reports the new speed late. The distance it has driven meanwhile is added when it does, so the
  // position is right in the end, but it is behind while the car speeds up and ahead while it brakes.
  double const bySpeed = DriveWithLateSpeed(false /* accelerometer */);
  // The accelerometer of the box feels the change at once.
  double const byAccel = DriveWithLateSpeed(true /* accelerometer */);
  LOG(LWARNING, ("The largest error by the car speed", bySpeed, "m, with the accelerometer", byAccel, "m"));
  TEST_GREATER(bySpeed, 0.8, ());
  TEST_LESS(bySpeed, 3, ());
  TEST_LESS(byAccel, bySpeed / 2, ());
}
}  // namespace nogps_engine_tests
