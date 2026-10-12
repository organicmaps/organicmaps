#include "map/nogps/bend_matcher.hpp"

#include "map/nogps/geo.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace nogps
{
namespace
{
double constexpr kBendShiftStepM = 1;
double constexpr kBendHistoryStepM = 2;
double constexpr kMaxRoadBendDeg = 45;
double constexpr kBendRoadSnapRadiusM = 8;

// Returns the directions of the road the car drives along at every step from the position, forward or back,
// continuous: without jumps at 360 degrees. Nothing if the road ends or turns sharply earlier.
std::optional<std::vector<double>> WalkRoadBearings(Roads & roads, ms::LatLon const & position, double bearingDeg,
                                                    int steps, bool forward)
{
  auto const start = roads.Snap(position, bearingDeg, kBendRoadSnapRadiusM);
  if (!start)
    return {};
  std::vector<double> result(steps + 1);
  ms::LatLon pos = start->m_point;
  // Where the car looks at, also when walking back.
  double carBearing = Orient(start->m_bearingDeg, bearingDeg);
  result[0] = carBearing;
  for (int i = 1; i <= steps; ++i)
  {
    double const moveBearing = forward ? carBearing : carBearing + 180;
    auto const road = roads.Snap(Move(pos, moveBearing, BendMatcher::kStepM), moveBearing, kBendRoadSnapRadiusM);
    if (!road)
      return {};
    double const roadBearing = Orient(road->m_bearingDeg, carBearing);
    double const bend = AngleDiff(roadBearing, carBearing);
    if (std::fabs(bend) > kMaxRoadBendDeg)
      return {};
    pos = road->m_point;
    carBearing = roadBearing;
    result[i] = result[i - 1] + bend;
  }
  return result;
}

// |roadStart| is the index in |road| of the place the car is at, may be fractional. Returns how different the
// rotation of the car and the shape of the road are, degrees. A constant difference doesn't count: the heading
// of the car may be wrong by some degrees.
double BendDifference(std::vector<double> const & car, std::vector<double> const & road, double roadStart)
{
  double sum = 0;
  double sumSquares = 0;
  for (size_t i = 0; i < car.size(); ++i)
  {
    double const position = roadStart + i;
    auto const index = static_cast<size_t>(std::floor(position));
    double const fraction = position - index;
    double const roadBearing =
        index + 1 < road.size() ? road[index] + (road[index + 1] - road[index]) * fraction : road.back();
    double const diff = car[i] - roadBearing;
    sum += diff;
    sumSquares += diff * diff;
  }
  double const mean = sum / car.size();
  return std::sqrt(std::max(0.0, sumSquares / car.size() - mean * mean));
}
}  // namespace

void BendMatcher::OnMotion(double yawDeltaDeg, double distanceM)
{
  m_yawDeg += yawDeltaDeg;
  m_odometerM += distanceM;
  if (!m_history.empty() && m_odometerM - m_history.back().m_odometerM < kBendHistoryStepM)
    return;
  m_history.push_back({m_odometerM, m_yawDeg});
  // One point at the start of the window or before it is kept.
  while (m_history.size() > 2 && m_odometerM - m_history[1].m_odometerM >= kWindowM)
    m_history.pop_front();
}

std::optional<double> BendMatcher::Match(Roads & roads, ms::LatLon const & position, double bearingDeg,
                                         double maxShiftM) const
{
  auto const count = static_cast<int>(kWindowM / kStepM) + 1;
  if (m_history.empty() || m_odometerM - m_history.front().m_odometerM < kWindowM)
    return {};

  // The rotation of the car at every step back from now.
  std::vector<double> car(count);
  for (int i = 0; i < count; ++i)
    car[i] = YawAt(m_odometerM - i * kStepM);
  auto const [min, max] = std::minmax_element(car.cbegin(), car.cend());
  if (*max - *min < kMinBendDeg)
    return {};

  // The direction of the road at every step from maxShiftM ahead of the position to the end of the window
  // behind the car, were the position ahead of the car by maxShiftM.
  auto const shiftSteps = static_cast<int>(std::ceil(maxShiftM / kStepM));
  auto const ahead = WalkRoadBearings(roads, position, bearingDeg, shiftSteps, true /* forward */);
  auto const behind = WalkRoadBearings(roads, position, bearingDeg, count - 1 + shiftSteps, false /* forward */);
  if (!ahead || !behind)
    return {};
  // road[shiftSteps] is the position, larger indexes are behind it.
  std::vector<double> road(ahead->size() + behind->size() - 1);
  for (size_t i = 0; i < ahead->size(); ++i)
    road[shiftSteps - i] = (*ahead)[i];
  for (size_t i = 1; i < behind->size(); ++i)
    road[shiftSteps + i] = (*behind)[i];

  double best = std::numeric_limits<double>::max();
  double bestShift = 0;
  auto const shifts = static_cast<int>(std::round(maxShiftM / kBendShiftStepM));
  std::vector<double> diffs(2 * shifts + 1);
  for (int s = -shifts; s <= shifts; ++s)
  {
    diffs[s + shifts] = BendDifference(car, road, shiftSteps + s * kBendShiftStepM / kStepM);
    if (diffs[s + shifts] < best)
    {
      best = diffs[s + shifts];
      bestShift = s * kBendShiftStepM;
    }
  }
  if (best > kMaxDiffDeg || std::fabs(bestShift) >= maxShiftM)
    return {};
  auto const aside = static_cast<int>(std::round(kClearAsideM / kBendShiftStepM));
  int const bestIndex = static_cast<int>(std::round(bestShift / kBendShiftStepM)) + shifts;
  for (int i = 0; i < static_cast<int>(diffs.size()); ++i)
    if (std::abs(i - bestIndex) >= aside && diffs[i] < kClearRatio * std::max(best, kMinAsideDiffDeg))
      return {};
  return bestShift;
}

double BendMatcher::YawAt(double odometerM) const
{
  Sample const * previous = nullptr;
  for (auto const & point : m_history)
  {
    if (point.m_odometerM >= odometerM)
    {
      if (previous == nullptr || point.m_odometerM == previous->m_odometerM)
        return point.m_yawDeg;
      return previous->m_yawDeg + (point.m_yawDeg - previous->m_yawDeg) * (odometerM - previous->m_odometerM) /
                                      (point.m_odometerM - previous->m_odometerM);
    }
    previous = &point;
  }
  return previous != nullptr ? previous->m_yawDeg : 0;
}
}  // namespace nogps
