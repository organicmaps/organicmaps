#include "map/relation_track.hpp"

#include "drape_frontend/relations_draw_info.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/spatial_hash_grid.hpp"

#include <deque>

namespace relation_track_merger  // Unity build protect
{
namespace  // Avoid exposing symbols
{
using TrackGeometry = RelationTrackBuilder::TrackGeometry;

bool IsEqual(m2::PointD const & lhs, m2::PointD const & rhs)
{
  return lhs.EqualDxDy(rhs, kMwmPointAccuracy);
}

struct EndpointRef
{
  size_t memberIdx;
  bool isFront;  // true if this endpoint is the front (first point) of the member.
};

/// Maintains endpoint map and used-state for chain building.
class Merger
{
public:
  explicit Merger(std::vector<TrackGeometry> const & members)
    : m_members(members)
    , m_used(members.size(), false)
    , m_endpointMap(kMwmPointAccuracy)
  {
    for (size_t i = 0; i < m_members.size(); ++i)
    {
      auto const & m = m_members[i];
      ASSERT_GREATER(m.size(), 1, ());

      m_endpointMap.Emplace(m.front().GetPoint(), EndpointRef{i, true});
      m_endpointMap.Emplace(m.back().GetPoint(), EndpointRef{i, false});
    }
  }

  /// Builds the longest connected chain from @p startIdx, growing in both directions.
  /// Marks consumed members as used.
  TrackGeometry BuildChain(size_t startIdx)
  {
    ASSERT_LESS(startIdx, m_members.size(), ());
    ASSERT(!m_used[startIdx], ());

    m_used[startIdx] = true;
    std::deque<geometry::PointWithAltitude> chain(m_members[startIdx].begin(), m_members[startIdx].end());

    // Grow forward (from back of chain).
    size_t lastFwd = startIdx;
    while (auto const * ref = FindEndpoint(chain.back().GetPoint(), lastFwd))
    {
      auto const & m = m_members[ref->memberIdx];
      lastFwd = ref->memberIdx;
      m_used[ref->memberIdx] = true;

      if (ref->isFront)
        chain.insert(chain.end(), m.begin() + 1, m.end());
      else
        chain.insert(chain.end(), m.rbegin() + 1, m.rend());
    }

    // Grow backward (from front of chain).
    size_t lastBwd = startIdx;
    while (auto const * ref = FindEndpoint(chain.front().GetPoint(), lastBwd))
    {
      auto const & m = m_members[ref->memberIdx];
      lastBwd = ref->memberIdx;
      m_used[ref->memberIdx] = true;

      if (ref->isFront)
        chain.insert(chain.begin(), m.rbegin(), m.rend() - 1);
      else
        chain.insert(chain.begin(), m.begin(), m.end() - 1);
    }

    return TrackGeometry(chain.begin(), chain.end());
  }

  bool IsUsed(size_t idx) const { return m_used[idx]; }
  size_t Size() const { return m_members.size(); }

  /// Checks if @p member connects to @p pt (front or back within kMwmPointAccuracy).
  /// If connected, returns true and sets @p needReverse.
  static bool Connects(TrackGeometry const & member, m2::PointD const & pt, bool & needReverse)
  {
    if (IsEqual(member.front().GetPoint(), pt))
    {
      needReverse = false;
      return true;
    }
    if (IsEqual(member.back().GetPoint(), pt))
    {
      needReverse = true;
      return true;
    }
    return false;
  }

private:
  /// Finds an unused endpoint matching @p pt, preferring members closest to @p lastIdx.
  EndpointRef const * FindEndpoint(m2::PointD const & pt, size_t lastIdx) const
  {
    EndpointRef const * best = nullptr;
    size_t bestDelta = std::numeric_limits<size_t>::max();

    m_endpointMap.ForEachPoint(pt, [&](EndpointRef const & ref)
    {
      if (m_used[ref.memberIdx])
        return;

      size_t const delta = math::AbsDiff(ref.memberIdx, lastIdx);
      if (delta < bestDelta)
      {
        bestDelta = delta;
        best = &ref;
      }
    });

    return best;
  }

  std::vector<TrackGeometry> const & m_members;
  std::vector<bool> m_used;
  m2::PointHashMap<EndpointRef> m_endpointMap;
};
}  // namespace
}  // namespace relation_track_merger

// RelationTrackBuilder implementation.

RelationTrackBuilder::RelationTrackBuilder(DataSource const & dataSource, FeatureID const & fid)
  : m_dataSource(dataSource)
  , m_fid(fid)
{}

std::optional<RelationTrackBuilder::Data> RelationTrackBuilder::Build()
{
  df::RelationsDrawSettings sett;
  sett.Load();
  if (sett.IsEmpty())
    return std::nullopt;

  FeaturesLoaderGuard guard(m_dataSource, m_fid.m_mwmId);
  auto ft = guard.GetFeatureByIndex(m_fid.m_index);
  ASSERT(ft, ());

  for (uint32_t const relID : ft->GetRelations())
  {
    if (!sett.MatchHikingOrCycling(ft->ReadRelationType(relID)))
      continue;

    auto const rel = ft->ReadRelation<feature::RouteRelation>(relID);

    size_t startIdx = 0;
    auto members = LoadMemberGeometries(rel, startIdx);
    if (members.empty() || startIdx >= members.size())
      continue;

    auto lines = MergeAllMembers(members);
    if (lines.empty())
      continue;

    Data data;
    data.m_lines = std::move(lines);
    data.m_name = std::string(rel.GetDefaultName());
    data.m_color = rel.GetColor();
    return data;
  }
  return std::nullopt;
}

std::optional<df::TransitInfo> RelationTrackBuilder::BuildTransitInfo(uint32_t relID)
{
  FeaturesLoaderGuard guard(m_dataSource, m_fid.m_mwmId);
  auto ft = guard.GetFeatureByIndex(m_fid.m_index);
  ASSERT(ft, ());

  auto const rel = ft->ReadRelation<feature::RouteRelation>(relID);

  df::TransitInfo info;
  info.m_color = rel.GetColor();

  // Collect lines (reuses existing LoadMemberGeometries + MergeOrdered).
  size_t startIdx = 0;
  auto members = LoadMemberGeometries(rel, startIdx);
  if (!members.empty())
  {
    auto const lines = MergeOrdered(members);
    info.m_routes.reserve(lines.size());
    for (auto const & line : lines)
    {
      df::TransitInfo::Route route;
      route.m_polyline.reserve(line.size());
      for (auto const & p : line)
        route.m_polyline.push_back(p.GetPoint());
      info.m_routes.push_back(std::move(route));
    }
  }

  for (uint32_t const ftIdx : rel.GetMembers())
  {
    auto stopFt = guard.GetFeatureByIndex(ftIdx);
    if (!stopFt || stopFt->GetGeomType() != feature::GeomType::Point)
      continue;

    df::TransitInfo::Stop stop;
    stop.m_featureID = stopFt->GetID();
    stop.m_pos = stopFt->GetCenter();
    stop.m_name = std::string(stopFt->GetReadableName());
    stop.m_highlight = (ftIdx == m_fid.m_index);  // Current (PP's) stop.
    info.m_stops.push_back(std::move(stop));
  }

  // Terminals: first and last point members of the relation.
  if (!info.m_stops.empty())
  {
    info.m_stops.front().m_highlight = true;
    info.m_stops.back().m_highlight = true;
  }

  if (info.IsEmpty())
    return std::nullopt;
  return info;
}

std::optional<df::SelectionInfo> RelationTrackBuilder::BuildSelectionInfo(uint32_t relID)
{
  FeaturesLoaderGuard guard(m_dataSource, m_fid.m_mwmId);
  auto ft = guard.GetFeatureByIndex(m_fid.m_index);
  ASSERT(ft, ());

  auto const rel = ft->ReadRelation<feature::RouteRelation>(relID);

  size_t startIdx = 0;
  auto members = LoadMemberGeometries(rel, startIdx);
  if (members.empty())
    return std::nullopt;

  auto lines = MergeOrdered(members);
  if (lines.empty())
    return std::nullopt;

  df::SelectionInfo info;
  info.m_color = rel.GetColor();
  info.m_lines.reserve(lines.size());
  for (auto const & line : lines)
  {
    df::Polyline polyline;
    polyline.reserve(line.size());
    for (auto const & p : line)
      polyline.push_back(p.GetPoint());
    info.m_lines.push_back(std::move(polyline));
  }
  return info;
}

std::vector<RelationTrackBuilder::TrackGeometry> RelationTrackBuilder::LoadMemberGeometries(
    feature::RouteRelation const & relation, size_t & startIdx)
{
  startIdx = std::numeric_limits<size_t>::max();

  auto const & ftMembers = relation.GetMembers();
  if (ftMembers.empty())
    return {};

  auto const handle = m_dataSource.GetMwmHandleById(m_fid.m_mwmId);
  if (!handle.IsAlive())
    return {};

  FeaturesLoaderGuard guard(m_dataSource, m_fid.m_mwmId);
  feature::AltitudeLoaderBase altLoader(*handle.GetValue());

  std::vector<TrackGeometry> result;
  result.reserve(ftMembers.size());

  for (uint32_t const ftIdx : ftMembers)
  {
    auto ft = guard.GetFeatureByIndex(ftIdx);
    ASSERT(ft, ());
    if (ft->GetGeomType() != feature::GeomType::Line)
      continue;

    ft->ParseGeometry(scales::GetUpperScale());
    size_t const pointsCount = ft->GetPointsCount();
    ASSERT_GREATER(pointsCount, 1, ());

    geometry::Altitudes const alts = altLoader.GetAltitudes(ftIdx, pointsCount);
    ASSERT_EQUAL(pointsCount, alts.size(), ());

    TrackGeometry line;
    line.reserve(pointsCount);
    line.emplace_back(ft->GetPoint(0), alts[0]);
    for (size_t j = 1; j < pointsCount; ++j)
    {
      /// @todo We still have equal points in Line Feature from MWM.
      if (!relation_track_merger::IsEqual(line.back().GetPoint(), ft->GetPoint(j)))
        line.emplace_back(ft->GetPoint(j), alts[j]);
    }

    if (line.size() < 2)
      continue;

    if (ftIdx == m_fid.m_index)
      startIdx = result.size();

    result.push_back(std::move(line));
  }
  return result;
}

RelationTrackBuilder::TrackGeometry RelationTrackBuilder::BuildChain(std::vector<TrackGeometry> const & members,
                                                                     size_t startIdx)
{
  relation_track_merger::Merger merger(members);
  return merger.BuildChain(startIdx);
}

std::vector<RelationTrackBuilder::TrackGeometry> RelationTrackBuilder::MergeAllMembers(
    std::vector<TrackGeometry> const & members)
{
  relation_track_merger::Merger merger(members);
  std::vector<TrackGeometry> chains;

  for (size_t i = 0; i < merger.Size(); ++i)
  {
    if (merger.IsUsed(i))
      continue;

    chains.push_back(merger.BuildChain(i));
  }

  if (chains.size() <= 1)
    return chains;

  // Keep chains[0] first (preserving relation member order), but reorder the rest
  // so that each next one is closest to the previous one's end. Reverse chains as needed.

  auto const FindNearest = [&chains](m2::PointD const & pt, std::vector<bool> const & picked)
  {
    struct Best
    {
      size_t idx = 0;
      double dist = std::numeric_limits<double>::max();
      bool reverse = false;
    } best;

    for (size_t j = 0; j < chains.size(); ++j)
    {
      if (picked[j])
        continue;

      auto const distFront = pt.SquaredLength(chains[j].front().GetPoint());
      auto const distBack = pt.SquaredLength(chains[j].back().GetPoint());

      if (distFront < best.dist)
        best = {j, distFront, false};
      if (distBack < best.dist)
        best = {j, distBack, true};
    }
    return best;
  };

  std::vector<TrackGeometry> result;
  result.reserve(chains.size());
  std::vector<bool> picked(chains.size(), false);
  picked[0] = true;

  // Check if reversing chains[0] gives a better connection to the nearest second chain.
  auto const fwd = FindNearest(chains[0].back().GetPoint(), picked);
  auto const rev = FindNearest(chains[0].front().GetPoint(), picked);
  if (rev.dist < fwd.dist)
    std::reverse(chains[0].begin(), chains[0].end());

  result.push_back(std::move(chains[0]));

  for (size_t step = 1; step < chains.size(); ++step)
  {
    auto best = FindNearest(result.back().back().GetPoint(), picked);

    picked[best.idx] = true;
    if (best.reverse)
      std::reverse(chains[best.idx].begin(), chains[best.idx].end());
    result.push_back(std::move(chains[best.idx]));
  }

  return result;
}

std::vector<RelationTrackBuilder::TrackGeometry> RelationTrackBuilder::MergeOrdered(
    std::vector<TrackGeometry> const & members)
{
  using relation_track_merger::Merger;

  std::vector<TrackGeometry> result;
  TrackGeometry chain(members[0].begin(), members[0].end());

  // Check if reversing the first member gives a better connection to the second.
  if (members.size() > 1)
  {
    bool needReverse = false;
    bool const connectsBack = Merger::Connects(members[1], chain.back().GetPoint(), needReverse);
    bool const connectsFront = Merger::Connects(members[1], chain.front().GetPoint(), needReverse);
    if (!connectsBack && connectsFront)
      std::reverse(chain.begin(), chain.end());
  }

  for (size_t i = 1; i < members.size(); ++i)
  {
    auto const & m = members[i];
    ASSERT_GREATER(m.size(), 1, ());

    bool needReverse = false;
    if (Merger::Connects(m, chain.back().GetPoint(), needReverse))
    {
      if (!needReverse)
        chain.insert(chain.end(), m.begin() + 1, m.end());  // front connects → forward
      else
        chain.insert(chain.end(), m.rbegin() + 1, m.rend());  // back connects → reversed
    }
    else
    {
      // Gap detected — flush current chain and start a new one.
      result.push_back(std::move(chain));
      chain.assign(m.begin(), m.end());
    }
  }

  result.push_back(std::move(chain));
  return result;
}
