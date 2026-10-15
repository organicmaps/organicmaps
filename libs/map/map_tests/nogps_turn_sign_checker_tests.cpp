#include "testing/testing.hpp"

#include "map/nogps/turn_sign_checker.hpp"

#include <cstdint>
#include <optional>

namespace nogps_turn_sign_checker_tests
{
using nogps::TurnSignChecker;
using Result = TurnSignChecker::Result;

class Driver
{
public:
  /// Drives a turn by GPS once a second, the gyroscope turns the car by |gyroSign| times as much.
  /// \returns the last result of the checker.
  std::optional<Result> Turn(double turnDeg, double gyroSign)
  {
    std::optional<Result> result;
    // Straight before the turn.
    for (int i = 0; i < 3; ++i)
      result = Drive(0, gyroSign);
    for (int i = 0; i < 6; ++i)
      result = Drive(turnDeg / 6, gyroSign);
    for (int i = 0; i < 3; ++i)
      if (auto const r = Drive(0, gyroSign))
        result = r;
    return result;
  }

  std::optional<Result> Drive(double stepDeg, double gyroSign)
  {
    // A small drift of the gyroscope on straight roads too.
    m_checker.OnGyro(gyroSign * stepDeg + 0.1);
    m_bearingDeg += stepDeg;
    m_timeMs += 1000;
    return m_checker.OnGpsBearing(m_bearingDeg, m_timeMs);
  }

  TurnSignChecker m_checker;
  int64_t m_timeMs = 0;
  double m_bearingDeg = 0;
};

UNIT_TEST(NoGps_TurnSignChecker_ConfirmsGyroscopeTurningAsGps)
{
  Driver driver;
  driver.Turn(90, 1);
  driver.Turn(-90, 1);
  TEST_EQUAL(driver.Turn(90, 1), Result::Ok, ());
  TEST_EQUAL(driver.m_checker.GetSameTurns(), 3, ());
}

UNIT_TEST(NoGps_TurnSignChecker_DetectsReversedGyroscope)
{
  Driver driver;
  TEST(!driver.Turn(90, -1), ());
  TEST(!driver.Turn(-45, -1), ());
  TEST_EQUAL(driver.Turn(90, -1), Result::Reversed, ());
}

UNIT_TEST(NoGps_TurnSignChecker_IgnoresDriftOnStraightRoads)
{
  Driver driver;
  for (int i = 0; i < 600; ++i)
    driver.Drive(0, 1);
  TEST_EQUAL(driver.m_checker.GetResult(), Result::Unknown, ());
  TEST_EQUAL(driver.m_checker.GetSameTurns() + driver.m_checker.GetOppositeTurns(), 0, ());
}

UNIT_TEST(NoGps_TurnSignChecker_IgnoresTurnsWithoutGps)
{
  Driver driver;
  driver.Drive(0, 1);
  // GPS is lost in the middle of a turn: the turn doesn't count.
  driver.m_checker.OnNoGpsBearing();
  driver.m_checker.OnGyro(-90);
  driver.m_timeMs += 10'000;
  driver.m_bearingDeg += 90;
  for (int i = 0; i < 3; ++i)
    driver.Drive(0, 1);
  TEST_EQUAL(driver.m_checker.GetSameTurns() + driver.m_checker.GetOppositeTurns(), 0, ());
}
}  // namespace nogps_turn_sign_checker_tests
