#include "map/track_elevation.hpp"

#include "map/track_statistics.hpp"

#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"

#include <algorithm>
#include <cmath>

void TrackElevation::Totals::Add(Totals const & other)
{
  m_ascent += other.m_ascent;
  m_descent += other.m_descent;
  m_smallAscent += other.m_smallAscent;
  m_smallDescent += other.m_smallDescent;
  m_min = std::min(m_min, other.m_min);
  m_max = std::max(m_max, other.m_max);
}

void TrackElevation::Totals::Apply(TrackStatistics & statistics) const
{
  if (m_min > m_max)
    return;
  bool const filtered = m_ascent != 0 || m_descent != 0;
  statistics.m_ascent = filtered ? m_ascent : m_smallAscent;
  statistics.m_descent = filtered ? m_descent : m_smallDescent;
  statistics.m_minElevation = m_min;
  statistics.m_maxElevation = m_max;
}

void TrackElevation::SmallVariation::Add(Altitude altitude)
{
  // Window anchors must not turn 1–2 m GPS jitter into gain on otherwise flat tracks.
  double constexpr kThreshold = 2 * kDeviation;
  if (!m_initialized)
  {
    m_initialized = true;
    m_low = m_high = altitude;
  }
  else if (m_direction == 0)
  {
    m_low = std::min(m_low, int(altitude));
    m_high = std::max(m_high, int(altitude));
    if (altitude - m_low > kThreshold)
    {
      m_direction = 1;
      m_anchor = m_low;
      m_extreme = altitude;
    }
    else if (m_high - altitude > kThreshold)
    {
      m_direction = -1;
      m_anchor = m_high;
      m_extreme = altitude;
    }
  }
  else if (m_direction > 0)
  {
    if (altitude > m_extreme)
      m_extreme = altitude;
    else if (m_extreme - altitude > kThreshold)
    {
      m_ascent += m_extreme - m_anchor;
      m_direction = -1;
      m_anchor = m_extreme;
      m_extreme = altitude;
    }
  }
  else if (altitude < m_extreme)
    m_extreme = altitude;
  else if (altitude - m_extreme > kThreshold)
  {
    m_descent += m_anchor - m_extreme;
    m_direction = 1;
    m_anchor = m_extreme;
    m_extreme = altitude;
  }
}

uint32_t TrackElevation::SmallVariation::GetAscent() const
{
  return m_ascent + (m_direction > 0 ? m_extreme - m_anchor : 0);
}

uint32_t TrackElevation::SmallVariation::GetDescent() const
{
  return m_descent + (m_direction < 0 ? m_anchor - m_extreme : 0);
}

void TrackElevation::Accumulator::Add(Altitude altitude)
{
  m_small.Add(altitude);
  m_totals.m_min = std::min(m_totals.m_min, altitude);
  m_totals.m_max = std::max(m_totals.m_max, altitude);
  if (!m_initialized)
    m_initialized = true;
  else if (int const delta = altitude - m_reference; delta > ElevationInfo::kDefThresholdGPS)
    m_totals.m_ascent += delta;
  else if (delta < -ElevationInfo::kDefThresholdGPS)
    m_totals.m_descent += -delta;
  else
    return;
  m_reference = altitude;
}

TrackElevation::Totals TrackElevation::Accumulator::GetTotals() const
{
  auto totals = m_totals;
  totals.m_smallAscent = m_small.GetAscent();
  totals.m_smallDescent = m_small.GetDescent();
  return totals;
}

bool TrackElevation::IsOutlier(Point a, Point b, Point c)
{
  double const left = b.m_distance - a.m_distance;
  double const right = c.m_distance - b.m_distance;
  if (left < 1e-9 || right < 1e-9)
    return false;
  int const up = b.m_altitude - a.m_altitude;
  int const down = c.m_altitude - b.m_altitude;
  return (up > 0) != (down > 0) && (std::abs(up) > left || std::abs(down) > right);
}

void TrackElevation::Constrain(Point anchor, Point point, double & lower, double & upper)
{
  double const distance = point.m_distance - anchor.m_distance;
  int const delta = point.m_altitude - anchor.m_altitude;
  if (distance <= 1e-9)
  {
    if (std::abs(delta) >= kDeviation)
    {
      lower = std::numeric_limits<double>::max();
      upper = -lower;
    }
    return;
  }
  double const inverse = 1 / distance;
  lower = std::max(lower, (delta - kDeviation) * inverse);
  upper = std::min(upper, (delta + kDeviation) * inverse);
}

TrackElevation::Point TrackElevation::Interpolate(Point point, Point right) const
{
  double const distance = right.m_distance - m_lastGood.m_distance;
  if (distance >= 1e-9)
  {
    double const fraction = (point.m_distance - m_lastGood.m_distance) / distance;
    point.m_altitude = Altitude(double(m_lastGood.m_altitude) * (1 - fraction) + double(right.m_altitude) * fraction);
  }
  return point;
}

void TrackElevation::Simplify(std::span<Point const> points, Points & output)
{
  ASSERT(!points.empty(), ());
  ASSERT_LESS_OR_EQUAL(points.size(), kWindowIntervals + 1, ());
  auto const bounds =
      std::minmax_element(points.begin(), points.end(), [](Point a, Point b) { return a.m_altitude < b.m_altitude; });
  if (points.size() <= 2 || bounds.second->m_altitude - bounds.first->m_altitude <= kFlatNoiseRange)
  {
    output.push_back(points.front());
    if (points.size() > 1)
      output.push_back(points.back());
    return;
  }

  std::array<m2::PointD, kWindowIntervals + 1> input;
  std::transform(points.begin(), points.end(), input.begin(),
                 [](Point p) { return m2::PointD(p.m_distance, p.m_altitude); });
  auto const begin = input.begin(), end = begin + points.size();
  m2::SquaredDistanceFromSegmentToPoint distance;
  double constexpr kSquareEps = kDeviation * kDeviation;
  if (simpl::MaxDistance(begin, end - 1, distance).first < kSquareEps)
  {
    output.push_back(points.front());
    output.push_back(points.back());
    return;
  }

  std::array<simpl::SimplifyOptimalRes, kWindowIntervals + 1> scratch;
  buffer_vector<m2::PointD, kWindowIntervals + 1> simplified;
  SimplifyNearOptimal(
      20, begin, end, kSquareEps, distance,
      AccumulateSkipSmallTrg<decltype(distance), m2::PointD, decltype(simplified)>(distance, simplified, kSquareEps),
      scratch);
  for (auto const & p : simplified)
    output.push_back({p.x, Altitude(p.y)});
}

void TrackElevation::Confirm(Point point, Output const & output)
{
  if (m_size > 0)
    Constrain(m_tail[0], point, m_lowerSlope, m_upperSlope);
  m_minTail = std::min(m_minTail, point.m_altitude);
  m_maxTail = std::max(m_maxTail, point.m_altitude);
  m_tail[m_size++] = point;
  if (m_size != m_tail.size())
    return;

  Points simplified;
  Simplify(std::span(m_tail.data(), m_size), simplified);
  auto const finalized = std::span(simplified.data(), simplified.size()).subspan(HasPrefix() ? 1 : 0);
  for (auto const & p : finalized)
    m_prefix.Add(p.m_altitude);
  if (output)
    output(finalized);
  m_tail[0] = point;
  m_size = 1;
  m_minTail = m_maxTail = point.m_altitude;
  m_lowerSlope = -std::numeric_limits<double>::max();
  m_upperSlope = std::numeric_limits<double>::max();
}

void TrackElevation::Add(Point point, Output const & output)
{
  if (!m_previous)
  {
    Confirm(point, output);
    m_lastGood = point;
  }
  else if (m_before)
  {
    if (IsOutlier(*m_before, *m_previous, point))
    {
      if (!m_longOutliers)
      {
        if (m_outlierCount < m_outliers.size())
          m_outliers[m_outlierCount++] = *m_previous;
        else
        {
          // Bound pathological runs; their valid endpoints still define the connection.
          m_longOutliers = true;
          m_outlierCount = 0;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < m_outlierCount; ++i)
        Confirm(Interpolate(m_outliers[i], *m_previous), output);
      Confirm(*m_previous, output);
      m_lastGood = *m_previous;
      m_outlierCount = 0;
      m_longOutliers = false;
    }
  }
  m_before = m_previous;
  m_previous = point;
  m_dirty = true;
}

void TrackElevation::Update() const
{
  if (!m_dirty)
    return;
  m_preview.clear();
  auto statistics = m_prefix;
  if (m_size != 0)
  {
    auto minimum = m_minTail, maximum = m_maxTail;
    double lower = m_lowerSlope, upper = m_upperSlope;
    for (size_t i = 0; i < m_outlierCount; ++i)
    {
      auto const p = Interpolate(m_outliers[i], *m_previous);
      minimum = std::min(minimum, p.m_altitude);
      maximum = std::max(maximum, p.m_altitude);
      Constrain(m_tail[0], p, lower, upper);
    }
    auto const endpoint = m_before ? *m_previous : m_tail[m_size - 1];
    minimum = std::min(minimum, endpoint.m_altitude);
    maximum = std::max(maximum, endpoint.m_altitude);
    double const distance = endpoint.m_distance - m_tail[0].m_distance;
    double const slope = distance > 1e-9 ? (endpoint.m_altitude - m_tail[0].m_altitude) / distance : 0;
    // A valid vertical corridor also bounds the perpendicular simplification error.
    if (maximum - minimum <= kFlatNoiseRange || (distance > 1e-9 && slope > lower && slope < upper))
    {
      m_preview.push_back(m_tail[0]);
      if (m_before || m_size > 1)
        m_preview.push_back(endpoint);
    }
    else
    {
      Points pending(m_tail.begin(), m_tail.begin() + m_size);
      if (m_before)
      {
        for (size_t i = 0; i < m_outlierCount; ++i)
          pending.push_back(Interpolate(m_outliers[i], *m_previous));
        pending.push_back(*m_previous);
      }
      auto remaining = std::span<Point const>(pending.data(), pending.size());
      while (remaining.size() > kWindowIntervals + 1)
      {
        Simplify(remaining.first(kWindowIntervals + 1), m_preview);
        remaining = remaining.subspan(kWindowIntervals);
        m_preview.pop_back();
      }
      Simplify(remaining, m_preview);
    }
    for (size_t i = HasPrefix() ? 1 : 0; i < m_preview.size(); ++i)
      statistics.Add(m_preview[i].m_altitude);
  }
  m_totals = statistics.GetTotals();
  m_dirty = false;
}

TrackElevation::Totals const & TrackElevation::GetTotals() const
{
  Update();
  return m_totals;
}

std::span<TrackElevation::Point const> TrackElevation::GetTail() const
{
  Update();
  return {m_preview.data(), m_preview.size()};
}

TrackElevation::Totals TrackElevation::Process(std::vector<ElevationInfo::GeometryLine> const & lines,
                                               std::vector<ElevationInfo::Points> * profile,
                                               std::function<bool()> const & isCancelled)
{
  Totals totals;
  for (auto const & line : lines)
  {
    TrackElevation processor;
    ElevationInfo::Points * output = profile ? &profile->emplace_back() : nullptr;
    Output const sink = output ? Output([output](std::span<Point const> points)
    { output->insert(output->end(), points.begin(), points.end()); })
                               : Output{};
    double distance = 0;
    for (size_t i = 0; i < line.size(); ++i)
    {
      if (i % kWindowIntervals == 0 && isCancelled && isCancelled())
        return {};
      if (i > 0)
        distance += mercator::DistanceOnEarth(line[i - 1].GetPoint(), line[i].GetPoint());
      processor.Add({distance, line[i].GetAltitude()}, sink);
    }
    totals.Add(processor.GetTotals());
    if (output)
    {
      auto const tail = processor.GetTail().subspan(processor.HasPrefix() ? 1 : 0);
      output->insert(output->end(), tail.begin(), tail.end());
    }
  }
  return totals;
}
