#include "testing/testing.hpp"

#include "map/extrapolation/extrapolator.hpp"

#include "platform/location.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

namespace
{
using namespace location;
using namespace extrapolation;

void TestGpsInfo(GpsInfo const & tested, GpsInfo const & expected)
{
  double constexpr kEpsilon = 1e-5;
  TEST_EQUAL(tested.m_source, expected.m_source, ());
  TEST(AlmostEqualAbs(tested.m_timestamp, expected.m_timestamp, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_latitude, expected.m_latitude, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_longitude, expected.m_longitude, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_horizontalAccuracy, expected.m_horizontalAccuracy, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_altitude, expected.m_altitude, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_verticalAccuracy, expected.m_verticalAccuracy, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_bearing, expected.m_bearing, kEpsilon), ());
  TEST(AlmostEqualAbs(tested.m_speed, expected.m_speed, kEpsilon), ());
  TEST(tested.m_timing == expected.m_timing, ());
}

GpsInfo GetGpsInfo(double timestampS, double lat, double lon, double altitude, double speed)
{
  return GpsInfo{EAppleNative,
                 timestampS,
                 lat,
                 lon,
                 10.0 /* m_horizontalAccuracy */,
                 altitude,
                 10.0 /* m_verticalAccuracy */,
                 0.0 /* m_bearing */,
                 speed};
}

GpsInfo MovingFix(double timestamp)
{
  return GetGpsInfo(timestamp, 1.0 /* lat */, 10.0 + timestamp * 1e-4 /* lon */, 1.0 /* altitude */, 10.0 /* speed */);
}

GpsInfo TimedFix(double utc, int observedMilliseconds)
{
  auto info = MovingFix(1.0 + observedMilliseconds / 1000.0);
  info.m_timestamp = utc;
  auto const observed = Extrapolator::Clock::time_point(std::chrono::milliseconds(observedMilliseconds));
  info.m_timing = GpsInfo::Timing{observed, observed + std::chrono::seconds(5)};
  return info;
}

struct ExtrapolatorFixture
{
  using Task = Extrapolator::Task;
  using Milliseconds = std::chrono::milliseconds;

  struct PendingTask
  {
    Extrapolator::Clock::time_point m_when;
    Task m_task;
  };

  Extrapolator::Clock::time_point m_now{};
  std::vector<PendingTask> m_background;
  std::deque<Task> m_gui;
  std::vector<GpsInfo> m_updates;
  std::vector<UpdateType> m_updateTypes;
  std::function<void(GpsInfo const &)> m_onUpdate;
  std::unique_ptr<Extrapolator> m_extrapolator = std::make_unique<Extrapolator>(
      [this](GpsInfo const & info, UpdateType type)
  {
    m_updates.push_back(info);
    m_updateTypes.push_back(type);
    if (m_onUpdate)
      m_onUpdate(info);
  }, [this] { return m_now; },
      [this](Milliseconds delay, Task task) { m_background.push_back({m_now + delay, std::move(task)}); });

  void RunBackground()
  {
    while (true)
    {
      auto const it = std::find_if(m_background.begin(), m_background.end(),
                                   [this](PendingTask const & task) { return task.m_when <= m_now; });
      if (it == m_background.end())
        return;
      m_gui.push_back(std::move(it->m_task));
      m_background.erase(it);
    }
  }

  void RunGui()
  {
    while (!m_gui.empty())
    {
      Task task = std::move(m_gui.front());
      m_gui.pop_front();
      task();
    }
  }

  void Run()
  {
    RunBackground();
    RunGui();
  }

  void Advance(int milliseconds)
  {
    m_now += Milliseconds(milliseconds);
    Run();
  }

  void Update(GpsInfo const & info)
  {
    m_extrapolator->OnLocationUpdate(info);
    Run();
  }

  void StartMoving()
  {
    m_extrapolator->Enable(true);
    Update(MovingFix(1.0));
    Advance(1000);
    Update(MovingFix(2.0));
  }
};

UNIT_TEST(Extrapolator_EqualTimeCorrectionPreservesHistory)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.Advance(20);
  auto corrected = MovingFix(2.0);
  corrected.m_longitude += 5e-6;
  corrected.m_horizontalAccuracy = 9.0;
  f.Update(corrected);
  TestGpsInfo(f.m_updates.back(), corrected);
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(MovingFix(1.0), corrected, 220));
}

UNIT_TEST(Extrapolator_MonotonicTimeSurvivesUtcRollback)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  auto const first = TimedFix(1000, 0);
  auto const second = TimedFix(-1000, 1000);
  f.Update(first);
  f.Advance(1000);
  f.Update(second);
  TestGpsInfo(f.m_updates.back(), second);
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(first, second, 200));
  TEST(f.m_updates.back().m_timing == second.m_timing, ());
}

UNIT_TEST(Extrapolator_SameUtcWithAdvancingMonotonicTimeIsDistinct)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  auto const first = TimedFix(1000, 0);
  auto const second = TimedFix(1000, 1000);
  f.Update(first);
  f.Advance(1000);
  f.Update(second);
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(first, second, 200));
}

UNIT_TEST(Extrapolator_FastInputKeepsIntegerMeasurementPrecision)
{
  using namespace std::chrono_literals;
  ExtrapolatorFixture f;
  auto const observed = Extrapolator::Clock::time_point(24h * 30);
  f.m_now = observed;
  auto first = MovingFix(1);
  first.m_longitude = 10;
  first.m_timing = GpsInfo::Timing{observed, observed + 5s};
  auto second = first;
  second.m_longitude += 1e-6;
  second.m_timing = GpsInfo::Timing{observed + 50ms, observed + 5050ms};
  f.m_extrapolator->Enable(true);
  f.Update(first);
  f.Advance(50);
  f.Update(second);
  f.Advance(200);

  double const expected = second.m_longitude + 4 * (second.m_longitude - first.m_longitude);
  TEST_ALMOST_EQUAL_ABS(f.m_updates.back().m_longitude, expected, 1e-12, ());
  TEST(f.m_updates.back().m_timing == second.m_timing, ());
}

UNIT_TEST(Extrapolator_MonotonicClockRejectsDelayedAndFutureInput)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  auto const first = TimedFix(1000, 0);
  auto const second = TimedFix(1001, 1000);
  f.Update(first);
  f.Advance(1000);
  f.Update(second);
  auto const count = f.m_updates.size();
  f.Update(TimedFix(2000, 500));
  f.Update(TimedFix(3000, 2000));
  TEST_EQUAL(f.m_updates.size(), count, ());
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(first, second, 200));
}

UNIT_TEST(Extrapolator_ClockDomainChangeClearsHistory)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.Update(TimedFix(3, 1000));
  auto const count = f.m_updates.size();
  f.Advance(200);
  TEST_EQUAL(f.m_updates.size(), count, ());
  f.Update(MovingFix(4));
  f.Advance(200);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
}

UNIT_TEST(Extrapolator_SuspendDoesNotRefreshObservationAge)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  f.Update(TimedFix(1000, 0));
  f.Advance(1000);
  f.Update(TimedFix(1001, 1000));
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_now += std::chrono::minutes(1);
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(!f.m_updates.back().IsFresh(f.m_now), ());
  TEST(f.m_updates.back().m_timing->m_observedAt == Extrapolator::Clock::time_point(std::chrono::seconds(1)), ());
}

UNIT_TEST(Extrapolator_OldMeasurementCannotBeginNewPredictionLifetime)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  f.m_now += std::chrono::seconds(10);
  f.Update(TimedFix(1000, 0));
  f.Update(TimedFix(1001, 1000));
  auto const count = f.m_updates.size();
  TEST_EQUAL(count, 2, ());
  f.Advance(200);
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(!f.m_updates.back().IsFresh(f.m_now), ());
}

UNIT_TEST(Extrapolator_DelayedMeasurementUsesRemainingPredictionLifetime)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  auto const first = TimedFix(1000, 0);
  auto const second = TimedFix(1001, 1000);
  f.Update(first);
  f.m_now += ExtrapolatorFixture::Milliseconds(2500);
  f.m_extrapolator->OnLocationUpdate(second);
  TEST_EQUAL(f.m_updates.size(), 2, ());
  TestGpsInfo(f.m_updates.back(), second);
  f.Advance(200);
  TEST_EQUAL(f.m_updates.size(), 3, ());
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(first, second, 1700));
  f.Advance(300);
  TEST_EQUAL(f.m_updates.size(), 3, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_HistoricalCorrectionUpdatesDisplayWithoutRenewingFreshness)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->Enable(true);
  f.Update(TimedFix(1000, 0));
  f.Advance(1000);
  auto corrected = TimedFix(1001, 1000);
  f.Update(corrected);
  f.Advance(6000);
  auto const count = f.m_updates.size();
  corrected.m_longitude += 5e-6;
  corrected.m_horizontalAccuracy = 9.0;
  f.m_extrapolator->OnLocationUpdate(corrected);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), corrected);
  TEST(!f.m_updates.back().IsFresh(f.m_now), ());
  f.m_extrapolator->Enable(false);
  f.m_extrapolator->Enable(true);
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_EnableDoesNotLoseReceivedFix)
{
  ExtrapolatorFixture f;
  f.m_extrapolator->OnLocationUpdate(MovingFix(1.0));
  TEST_EQUAL(f.m_updates.size(), 1, ());
  f.m_extrapolator->Enable(true);
  f.Run();
  TEST_EQUAL(f.m_updates.size(), 1, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(1.0));
}

UNIT_TEST(Extrapolator_ExpiredCorrectionStillUpdatesMeasuredPosition)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.Advance(2000);
  auto const count = f.m_updates.size();
  auto corrected = MovingFix(2.0);
  corrected.m_longitude += 5e-6;
  corrected.m_horizontalAccuracy = 9.0;
  f.Update(corrected);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), corrected);
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_CallbackProvenanceMatchesReceivedAndPredictedPositions)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  TEST(f.m_updateTypes.back() == UpdateType::Measurement, ());
  f.Advance(200);
  TEST(f.m_updateTypes.back() == UpdateType::Prediction, ());

  auto correction = MovingFix(2.0);
  correction.m_horizontalAccuracy = 9.0;
  auto const count = f.m_updates.size();
  f.m_extrapolator->OnLocationUpdate(correction);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TEST(f.m_updateTypes.back() == UpdateType::Measurement, ());
  TestGpsInfo(f.m_updates.back(), correction);
  f.Advance(200);
  TEST(f.m_updateTypes.back() == UpdateType::Prediction, ());

  f.Advance(2000);
  correction.m_horizontalAccuracy = 8.0;
  f.m_extrapolator->OnLocationUpdate(correction);
  TEST(f.m_updateTypes.back() == UpdateType::Measurement, ());
  TestGpsInfo(f.m_updates.back(), correction);
}

UNIT_TEST(Extrapolator_ExactRepeatsDoNotExtendPrediction)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.Advance(1900);
  auto const count = f.m_updates.size();
  f.Update(MovingFix(2.0));
  TEST_EQUAL(f.m_updates.size(), count, ());
  f.Advance(201);
  f.Update(MovingFix(2.0));
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_CorrectionsDoNotExtendPrediction)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.Advance(1800);
  auto corrected = MovingFix(2.0);
  corrected.m_horizontalAccuracy = 9.0;
  f.Update(corrected);
  auto const count = f.m_updates.size();
  f.Advance(200);
  corrected.m_horizontalAccuracy = 8.0;
  f.Update(corrected);
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), corrected);
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_DelayedGuiUsesElapsedTime)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  f.m_now += ExtrapolatorFixture::Milliseconds(700);
  f.RunGui();
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(MovingFix(1.0), MovingFix(2.0), 900));

  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_now += ExtrapolatorFixture::Milliseconds(1000);
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_MeasurementsAreDeliveredBeforePredictionQueue)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  auto const count = f.m_updates.size();
  f.m_extrapolator->OnLocationUpdate(MovingFix(3.0));
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(3.0));
  auto corrected = MovingFix(3.0);
  corrected.m_longitude += 5e-6;
  f.m_extrapolator->OnLocationUpdate(corrected);
  TEST_EQUAL(f.m_updates.size(), count + 2, ());
  TestGpsInfo(f.m_updates.back(), corrected);
  f.m_now += ExtrapolatorFixture::Milliseconds(400);
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count + 2, ());
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(MovingFix(2.0), corrected, 600));
}

UNIT_TEST(Extrapolator_NewerFixInvalidatesQueuedGuiPrediction)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_extrapolator->OnLocationUpdate(MovingFix(3.0));
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  f.Run();
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(3.0));
}

UNIT_TEST(Extrapolator_ResetInvalidatesQueuedPredictionAndHistory)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_extrapolator->Reset();
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count, ());
  f.Update(MovingFix(3.0));
  f.Advance(200);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(3.0));
}

UNIT_TEST(Extrapolator_EnableChangesPreserveReceivedMeasurements)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_extrapolator->Enable(false);
  f.RunGui();
  TEST_EQUAL(f.m_updates.size(), count, ());
  f.Update(MovingFix(3.0));
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  f.m_extrapolator->Enable(true);
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(MovingFix(2.0), MovingFix(3.0), 200));
}

UNIT_TEST(Extrapolator_EnableDoesNotReviveExpiredPrediction)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_extrapolator->Enable(false);
  f.Advance(3000);
  auto const count = f.m_updates.size();
  f.m_extrapolator->Enable(true);
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_UtcRollbackStartsNewHistory)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  auto fix = MovingFix(3.0);
  fix.m_timestamp = -10.0;
  f.Update(fix);
  auto const count = f.m_updates.size();
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count, ());
  auto next = MovingFix(4.0);
  next.m_timestamp = -9.0;
  f.Update(next);
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(fix, next, 200));
}

UNIT_TEST(Extrapolator_DestructionInvalidatesQueuedCallbacks)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_now += ExtrapolatorFixture::Milliseconds(200);
  f.RunBackground();
  auto const count = f.m_updates.size();
  f.m_extrapolator.reset();
  f.RunGui();
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count, ());
}

UNIT_TEST(Extrapolator_DestructionInsideMeasurementCallbackIsSafe)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  auto const count = f.m_updates.size();
  f.m_onUpdate = [&f](GpsInfo const &) { f.m_extrapolator.reset(); };
  f.Update(MovingFix(3.0));
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(3.0));
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count + 1, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_EnableInsideMeasurementCallbackKeepsHistory)
{
  ExtrapolatorFixture f;
  f.m_onUpdate = [&f](GpsInfo const &) { f.m_extrapolator->Enable(true); };
  f.Update(MovingFix(1.0));
  f.m_onUpdate = {};
  f.Advance(1000);
  f.Update(MovingFix(2.0));
  f.Advance(200);
  TestGpsInfo(f.m_updates.back(), LinearExtrapolation(MovingFix(1.0), MovingFix(2.0), 200));
}

UNIT_TEST(Extrapolator_ReentrantMeasurementDoesNotMutateCallbackArgument)
{
  ExtrapolatorFixture f;
  bool nested = false;
  f.m_onUpdate = [&f, &nested](GpsInfo const & info)
  {
    if (!nested)
    {
      nested = true;
      f.m_extrapolator->OnLocationUpdate(MovingFix(2.0));
      TestGpsInfo(info, MovingFix(1.0));
    }
  };
  f.Update(MovingFix(1.0));
  TEST_EQUAL(f.m_updates.size(), 2, ());
  TestGpsInfo(f.m_updates.back(), MovingFix(2.0));
}

UNIT_TEST(Extrapolator_UndefinedFixStopsPredictionWithoutDelivery)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  auto const count = f.m_updates.size();
  f.Update(GpsInfo{});
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_NoIdleOrDisabledPredictionTasks)
{
  ExtrapolatorFixture f;
  TEST(f.m_background.empty(), ());
  f.Update(MovingFix(1.0));
  f.Advance(1000);
  f.Update(MovingFix(2.0));
  TEST_EQUAL(f.m_updates.size(), 2, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(Extrapolator_ResetInsideCallbackStopsPrediction)
{
  ExtrapolatorFixture f;
  f.StartMoving();
  f.m_onUpdate = [&f](GpsInfo const &) { f.m_extrapolator->Reset(); };
  f.Advance(200);
  auto const count = f.m_updates.size();
  f.Advance(1000);
  TEST_EQUAL(f.m_updates.size(), count, ());
  TEST(f.m_background.empty(), ());
}

UNIT_TEST(LinearExtrapolation)
{
  GpsInfo const loc1 = GetGpsInfo(0.0 /* timestampS */, 1.0 /* m_latitude */, 1.0 /* m_longitude */,
                                  1.0 /* m_altitude */, 10.0 /* m_speed */);
  GpsInfo const loc2 = GetGpsInfo(1.0 /* timestampS */, 1.01 /* m_latitude */, 1.01 /* m_longitude */,
                                  2.0 /* m_altitude */, 12.0 /* m_speed */);

  // 0 ms after |point2|.
  TestGpsInfo(LinearExtrapolation(loc1, loc2, 0 /* timeAfterPoint2Ms */), loc2);

  // 100 ms after |point2|.
  {
    GpsInfo const expected = GetGpsInfo(1.1 /* timestampS */, 1.011 /* m_latitude */, 1.011 /* m_longitude */,
                                        2.1 /* m_altitude */, 12.2 /* m_speed */);
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 100 /* timeAfterPoint2Ms */), expected);
  }

  // 200 ms after |point2|.
  {
    GpsInfo const expected = GetGpsInfo(1.2 /* timestampS */, 1.012 /* m_latitude */, 1.012 /* m_longitude */,
                                        2.2 /* m_altitude */, 12.4 /* m_speed */);
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 200 /* timeAfterPoint2Ms */), expected);
  }

  // 1000 ms after |point2|.
  {
    GpsInfo const expected = GetGpsInfo(2.0 /* timestampS */, 1.02 /* m_latitude */, 1.02 /* m_longitude */,
                                        3.0 /* m_altitude */, 14.0 /* m_speed */);
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 1000 /* timeAfterPoint2Ms */), expected);
  }
}

UNIT_TEST(AreCoordsGoodForExtrapolation)
{
  double constexpr kAltitude = 1.0;
  double constexpr kSpeed = 10.0;
  {
    GpsInfo loc1;
    GpsInfo loc2;
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are not valid."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999999 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999999 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Crossing meridian 180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999997 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, 179.999999 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian 180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999995 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, 179.999996 /* lon */, kAltitude, kSpeed);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian 180 but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, -179.999997 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999999 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian -180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 89.9997 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 89.9999 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to North Pole."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 89.9997 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 89.9998 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to North Pole but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, -89.9997 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, -89.9999 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to South Pole."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, -89.9997 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, -89.9998 /* lat */, -10.0 /* lon */, kAltitude, kSpeed);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to South Pole but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, -179.999995 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999996 /* lon */, kAltitude, kSpeed);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian -180 but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 10.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 2.0 /* lat */, 10.0 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are too far."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */, kAltitude, kSpeed);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are close enough."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Time is the same."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */, kAltitude, kSpeed);
    GpsInfo const loc2 = GetGpsInfo(3.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */, kAltitude, kSpeed);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Too rare locations."));
  }
}
}  // namespace
