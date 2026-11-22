#include "routing/speed_limit_finder.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace routing
{
namespace
{
size_t constexpr kMaxCachedMwms = 16;
double constexpr kTieBreakToleranceMeters = 2.0;
double constexpr kMinSegmentLengthEps = 1e-12;

m2::PointD BearingDegToMercatorDir(double bearingDeg)
{
  double const rad = math::DegToRad(bearingDeg);
  return {std::sin(rad), std::cos(rad)};
}

struct FeatureCandidate
{
  MwmSet::MwmId m_mwmId;
  uint32_t m_featureId = 0;
  RoadMatchCandidate m_match;
};
}  // namespace

std::optional<RoadMatchCandidate> MatchPointToSegment(m2::PointD const & pt, m2::PointD const & p0,
                                                      m2::PointD const & p1,
                                                      std::optional<m2::PointD> const & bearingDir, bool isOneWay,
                                                      double maxDistanceMeters)
{
  m2::PointD const segVec = p1 - p0;
  double const segLen = segVec.Length();
  if (segLen <= kMinSegmentLengthEps)
    return std::nullopt;

  m2::ParametrizedSegment<m2::PointD> const seg(p0, p1);
  m2::PointD const closest = seg.ClosestPointTo(pt);
  double const distM = mercator::DistanceOnEarth(pt, closest);
  if (distM > maxDistanceMeters)
    return std::nullopt;

  double score = distM;
  bool isForward = true;

  if (bearingDir.has_value())
  {
    m2::PointD const segDir = segVec / segLen;
    double const dot = std::clamp(m2::DotProduct(segDir, *bearingDir), -1.0, 1.0);
    isForward = dot >= 0.0;

    if (isOneWay)
    {
      double const alignment = std::max(0.0, dot);
      score += (1.0 - alignment) * kFreeRideHeadingPenaltyMeters;
      if (dot < 0.0)
        score += kFreeRideWrongWayPenaltyMeters;
    }
    else
    {
      score += (1.0 - std::abs(dot)) * kFreeRideHeadingPenaltyMeters;
    }
  }

  return RoadMatchCandidate{distM, score, isForward};
}

double SpeedLimitToMps(Maxspeed const & maxspeed, bool isForward, time_t now)
{
  if (!maxspeed.IsValid())
    return -1.0;

  SpeedInUnits const speedLimit = maxspeed.GetCurrentSpeed(now, isForward);
  if (speedLimit.IsNumeric())
  {
    double const speedKmPH =
        measurement_utils::ToSpeedKmPH(static_cast<double>(speedLimit.GetSpeed()), speedLimit.GetUnits());
    return measurement_utils::KmphToMps(speedKmPH);
  }

  if (speedLimit.GetSpeed() == kNoneMaxSpeed)
    return 0.0;

  return -1.0;
}

SpeedLimitFinder::SpeedLimitFinder(DataSource const & dataSource) : m_dataSource(dataSource) {}

double SpeedLimitFinder::GetSpeedLimitMps(ms::LatLon const & pos, std::optional<double> bearingDeg, time_t now) const
{
  if (!std::isfinite(pos.m_lat) || !std::isfinite(pos.m_lon))
    return -1.0;

  std::optional<m2::PointD> bearingDir;
  if (bearingDeg.has_value() && std::isfinite(*bearingDeg))
    bearingDir = BearingDegToMercatorDir(*bearingDeg);

  m2::PointD const pt = mercator::FromLatLon(pos);
  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(pt, kMaxFreeRideRoadDistanceMeters);

  auto const & isWay = ftypes::IsWayChecker::Instance();
  auto const & carModel = CarModel::AllLimitsInstance();

  std::vector<FeatureCandidate> candidates;

  m_dataSource.ForEachInRect([&](FeatureType & ft)
  {
    if (ft.GetGeomType() != feature::GeomType::Line)
      return;

    auto const & mwmId = ft.GetID().m_mwmId;
    if (!mwmId.IsAlive())
      return;

    auto const info = mwmId.GetInfo();
    if (!info || info->GetType() != MwmInfo::COUNTRY)
      return;

    if (!isWay(ft))
      return;

    feature::TypesHolder const types(ft);
    if (!carModel.IsRoad(types))
      return;

    bool const isOneWay = carModel.IsOneWay(types);
    std::optional<RoadMatchCandidate> bestSegment;

    ft.ForEachSegment([&](m2::PointD const & p0, m2::PointD const & p1)
    {
      auto const match = MatchPointToSegment(pt, p0, p1, bearingDir, isOneWay, kMaxFreeRideRoadDistanceMeters);
      if (match.has_value() && (!bestSegment.has_value() || match->m_score < bestSegment->m_score))
        bestSegment = match;
    }, FeatureType::BEST_GEOMETRY);

    if (bestSegment.has_value())
      candidates.push_back(FeatureCandidate{mwmId, ft.GetID().m_index, *bestSegment});
  }, rect, scales::GetUpperScale());

  if (candidates.empty())
    return -1.0;

  std::sort(candidates.begin(), candidates.end(), [](FeatureCandidate const & lhs, FeatureCandidate const & rhs)
  { return lhs.m_match.m_score < rhs.m_match.m_score; });

  time_t const queryTime = (now != 0) ? now : std::time(nullptr);
  auto const & best = candidates.front();

  for (auto const & candidate : candidates)
  {
    // Only inspect near-identical overlapping roads beyond the primary match so a distant
    // tagged highway never leaks its limit onto an untagged side street.
    if (&candidate != &best)
    {
      if (candidate.m_match.m_score - best.m_match.m_score > kTieBreakToleranceMeters)
        break;
      if (std::abs(candidate.m_match.m_distanceMeters - best.m_match.m_distanceMeters) > kTieBreakToleranceMeters)
        continue;
    }

    auto const maxspeeds = GetOrLoadMaxspeeds(candidate.m_mwmId);
    if (!maxspeeds || maxspeeds->IsEmpty())
      continue;

    Maxspeed const maxspeed = maxspeeds->GetMaxspeed(candidate.m_featureId);
    if (maxspeed.IsValid())
      return SpeedLimitToMps(maxspeed, candidate.m_match.m_isForward, queryTime);
  }

  return -1.0;
}

void SpeedLimitFinder::OnMwmDeregistered(platform::LocalCountryFile const & localFile)
{
  std::lock_guard lock(m_cacheMutex);
  std::erase_if(m_cache, [&](auto const & entry)
  {
    auto const & id = entry.first;
    if (!id.IsAlive())
      return true;
    auto const info = id.GetInfo();
    return !info || info->GetLocalFile() == localFile;
  });
}

void SpeedLimitFinder::ClearCache()
{
  std::lock_guard lock(m_cacheMutex);
  m_cache.clear();
}

std::shared_ptr<Maxspeeds> SpeedLimitFinder::GetOrLoadMaxspeeds(MwmSet::MwmId const & mwmId) const
{
  if (!mwmId.IsAlive())
    return nullptr;

  {
    std::lock_guard lock(m_cacheMutex);
    auto const it = m_cache.find(mwmId);
    if (it != m_cache.end())
      return it->second;
  }

  auto const handle = m_dataSource.GetMwmHandleById(mwmId);
  if (!handle.IsAlive())
    return nullptr;

  std::shared_ptr<Maxspeeds> loaded = LoadMaxspeeds(handle);

  {
    std::lock_guard lock(m_cacheMutex);
    if (!mwmId.IsAlive())
      return nullptr;
    if (auto const it = m_cache.find(mwmId); it != m_cache.end())
      return it->second;
    std::erase_if(m_cache, [](auto const & entry) { return !entry.first.IsAlive(); });
    if (m_cache.size() >= kMaxCachedMwms)
      m_cache.erase(m_cache.begin());
    auto const [it, inserted] = m_cache.emplace(mwmId, std::move(loaded));
    return it->second;
  }
}
}  // namespace routing
