#include "map/relation_track.hpp"

#include "drape_frontend/relations_draw_info.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/spatial_hash_grid.hpp"

#include <deque>
#include <unordered_map>

namespace relation_track_merger  // Unity build protect
{
using TrackGeometry = RelationTrackBuilder::TrackGeometry;

m2::SpatialHashGrid const & GetPointGrid()
{
  static m2::SpatialHashGrid const grid(kMwmPointAccuracy);
  return grid;
}

using Grid = m2::SpatialHashGrid;

struct EndpointRef
{
  size_t memberIdx;
  bool isFront;  // true if this endpoint is the front (first point) of the member.
};

using EndpointMap = std::unordered_multimap<Grid::Cell, EndpointRef, Grid::Hash>;

/// Maintains endpoint map and used-state for chain building.
class Merger
{
public:
  explicit Merger(std::vector<TrackGeometry> const & members)
    : m_members(members)
    , m_used(members.size(), false)
    , m_grid(GetPointGrid())
  {
    for (size_t i = 0; i < m_members.size(); ++i)
    {
      auto const & m = m_members[i];
      ASSERT_GREATER(m.size(), 1, ());

      m_endpointMap.emplace(m_grid.ToCell(m.front().GetPoint()), EndpointRef{i, true});
      m_endpointMap.emplace(m_grid.ToCell(m.back().GetPoint()), EndpointRef{i, false});
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
    if (member.front().GetPoint().EqualDxDy(pt, kMwmPointAccuracy))
    {
      needReverse = false;
      return true;
    }
    if (member.back().GetPoint().EqualDxDy(pt, kMwmPointAccuracy))
    {
      needReverse = true;
      return true;
    }
    return false;
  }

private:
  /// Finds the closest unused matching endpoint near @p pt, preferring members closest to @p lastIdx.
  EndpointRef const * FindEndpoint(m2::PointD const & pt, size_t lastIdx) const
  {
    EndpointRef const * best = nullptr;
    size_t bestDelta = std::numeric_limits<size_t>::max();

    for (auto const & cell : m_grid.GetNearbyCells(pt))
    {
      auto const [rangeBegin, rangeEnd] = m_endpointMap.equal_range(cell);
      for (auto it = rangeBegin; it != rangeEnd; ++it)
      {
        if (m_used[it->second.memberIdx])
          continue;

        size_t const delta = math::AbsDiff(it->second.memberIdx, lastIdx);
        if (delta < bestDelta)
        {
          bestDelta = delta;
          best = &it->second;
        }
      }
    }
    return best;
  }

  std::vector<TrackGeometry> const & m_members;
  std::vector<bool> m_used;
  Grid const & m_grid;
  EndpointMap m_endpointMap;
};

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

    if (ftIdx == m_fid.m_index)
      startIdx = result.size();

    ft->ParseGeometry(scales::GetUpperScale());
    size_t const pointCount = ft->GetPointsCount();
    ASSERT_GREATER(pointCount, 1, ());

    geometry::Altitudes alts = altLoader.GetAltitudes(ftIdx, pointCount);
    ASSERT_EQUAL(pointCount, alts.size(), ());

    TrackGeometry line;
    line.reserve(pointCount);
    for (size_t j = 0; j < pointCount; ++j)
      line.emplace_back(ft->GetPoint(j), alts[j]);

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
