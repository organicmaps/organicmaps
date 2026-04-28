#include "map/relation_track.hpp"

#include "drape_frontend/relations_draw_info.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/spatial_hash_grid.hpp"

#include "defines.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_set>

namespace relation_track_merger  // Unity build protect
{
namespace  // Avoid exposing symbols
{
using Geometry = RelationTrackBuilder::Geometry;
using RelationID = RelationTrackBuilder::RelationID;

bool IsEqual(m2::PointD const & lhs, m2::PointD const & rhs)
{
  return lhs.EqualDxDy(rhs, kMwmPointAccuracy);
}

// The same OSM way can appear as a Feature in several MWMs (the borders are not strict
// cuts). Drop neighbour-side members that already match an existing member by point
// count, both endpoints, and (only then) all points.
bool IsSameLine(Geometry const & a, Geometry const & b)
{
  if (a.Size() != b.Size())
    return false;
  if (!IsEqual(a.FrontPoint(), b.FrontPoint()) || !IsEqual(a.BackPoint(), b.BackPoint()))
    return false;
  for (size_t i = 1; i + 1 < a.Size(); ++i)
    if (!IsEqual(a.m_points[i].GetPoint(), b.m_points[i].GetPoint()))
      return false;
  return true;
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
  explicit Merger(std::vector<Geometry> const & members)
    : m_members(members)
    , m_used(members.size(), false)
    , m_endpointMap(kMwmPointAccuracy)
  {
    for (size_t i = 0; i < m_members.size(); ++i)
    {
      auto const & m = m_members[i];
      ASSERT_GREATER(m.Size(), 1, ());

      m_endpointMap.Emplace(m.FrontPoint(), EndpointRef{i, true});
      m_endpointMap.Emplace(m.BackPoint(), EndpointRef{i, false});
    }
  }

  /// Builds the longest connected chain from @p startIdx, growing in both directions.
  /// Marks consumed members as used. Provenance (m_relIDs) is preserved end-to-end.
  Geometry BuildChain(size_t startIdx)
  {
    ASSERT_LESS(startIdx, m_members.size(), ());
    ASSERT(!m_used[startIdx], ());

    m_used[startIdx] = true;
    Geometry chain = m_members[startIdx];

    // Grow forward (append at back of chain).
    // ref->isFront == true: m's front equals chain's back → append m forward.
    // ref->isFront == false: m's back equals chain's back → append m reversed.
    size_t lastFwd = startIdx;
    while (auto const * ref = FindEndpoint(chain.BackPoint(), lastFwd))
    {
      lastFwd = ref->memberIdx;
      m_used[ref->memberIdx] = true;
      chain.Insert(m_members[ref->memberIdx], false /* atFront */, !ref->isFront /* reverse */);
    }

    // Grow backward (prepend at front of chain).
    // ref->isFront == true: m's front equals chain's front → prepend m reversed.
    // ref->isFront == false: m's back equals chain's front → prepend m forward.
    size_t lastBwd = startIdx;
    while (auto const * ref = FindEndpoint(chain.FrontPoint(), lastBwd))
    {
      lastBwd = ref->memberIdx;
      m_used[ref->memberIdx] = true;
      chain.Insert(m_members[ref->memberIdx], true /* atFront */, ref->isFront /* reverse */);
    }

    return chain;
  }

  bool IsUsed(size_t idx) const { return m_used[idx]; }
  size_t Size() const { return m_members.size(); }

  /// Checks if @p member connects to @p pt (front or back within kMwmPointAccuracy).
  /// If connected, returns true and sets @p needReverse.
  static bool Connects(Geometry const & member, m2::PointD const & pt, bool & needReverse)
  {
    if (IsEqual(member.FrontPoint(), pt))
    {
      needReverse = false;
      return true;
    }
    if (IsEqual(member.BackPoint(), pt))
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

  std::vector<Geometry> const & m_members;
  std::vector<bool> m_used;
  m2::PointHashMap<EndpointRef> m_endpointMap;
};

void ToPolyline(Geometry const & g, df::Polyline & poly)
{
  poly.reserve(g.Size());
  for (auto const & p : g.m_points)
    poly.push_back(p.GetPoint());
}
}  // namespace
}  // namespace relation_track_merger

// Geometry implementation.

void RelationTrackBuilder::Geometry::Insert(Geometry const & other, bool atFront, bool reverse)
{
  if (other.IsEmpty())
    return;

  // Splice points, dropping the shared seam point of @p other (front when appending,
  // back when prepending; flipped under @p reverse).

  // clang-format off
  auto const & p = other.m_points;
  if (!atFront)
  {
    if (!reverse)
      m_points.insert(m_points.end(), p.begin() + 1, p.end());  // [front+1 .. back]
    else
      m_points.insert(m_points.end(), p.rbegin() + 1, p.rend());  // [back-1 .. front]
  }
  else
  {
    if (!reverse)
      m_points.insert(m_points.begin(), p.begin(), p.end() - 1);  // [front .. back-1]
    else
      m_points.insert(m_points.begin(), p.rbegin(), p.rend() - 1);  // [back .. front+1]
  }
  // clang-format on

  // Splice provenance with matching orientation. Each ref names a whole run and
  // carries no per-run direction flag, so we just append/prepend in the right order.

  // clang-format off
  auto const & r = other.m_relIDs;
  if (!atFront)
  {
    // do not append the same consecutive ids
    if (!m_relIDs.empty() && r.size() == 1 && m_relIDs.back() == r.front())
      return;

    if (!reverse)
      m_relIDs.insert(m_relIDs.end(), r.begin(), r.end());
    else
      m_relIDs.insert(m_relIDs.end(), r.rbegin(), r.rend());
  }
  else
  {
    // do not append the same consecutive ids
    if (!m_relIDs.empty() && r.size() == 1 && m_relIDs.front() == r.front())
      return;

    if (!reverse)
      m_relIDs.insert(m_relIDs.begin(), r.begin(), r.end());
    else
      m_relIDs.insert(m_relIDs.begin(), r.rbegin(), r.rend());
  }
  // clang-format on
}

void RelationTrackBuilder::Geometry::Reverse()
{
  std::reverse(m_points.begin(), m_points.end());
  std::reverse(m_relIDs.begin(), m_relIDs.end());
}

// RelationTrackBuilder implementation.

RelationTrackBuilder::RelationTrackBuilder(DataSource const & dataSource, FeatureID const & fid,
                                           storage::CountryInfoGetter const * infoGetter)
  : m_dataSource(dataSource)
  , m_fid(fid)
  , m_infoGetter(infoGetter)
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
    auto members = LoadMemberGeometries(rel, guard, RelationID(m_fid.m_mwmId, relID));
    if (members.empty())
      continue;

    // Cross-MWM merging.
    AppendNeighbourMembers(guard, relID, members);

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

void RelationTrackBuilder::AppendNeighbourMembers(FeaturesLoaderGuard const & guard, uint32_t relIdx,
                                                  std::vector<Geometry> & members)
{
  if (!m_infoGetter)
    return;

  uint32_t osmRelID;
  auto const & cont = guard.GetContainer();
  if (cont.IsExist(RELATION_OSMIDS_FILE_TAG))
  {
    auto const tbl = feature::FeaturesOffsetsTable::Load(cont, RELATION_OSMIDS_FILE_TAG);
    osmRelID = tbl->GetFeatureOffset(relIdx);
  }
  else
    return;

  // BFS
  auto const mwmId = guard.GetId();
  std::queue<MwmSet::MwmId> frontier;
  frontier.push(mwmId);
  std::unordered_set<MwmSet::MwmId> visited;
  visited.insert(mwmId);

  while (!frontier.empty())
  {
    /// @todo Use m_infoGetter->GetLimitRectForLeaf (is more precise).
    /// But! It returns a FullRect for the obsolete MWMs.
    /// Make a better function to select an appropriate rect.
    auto const rect = frontier.front().GetInfo()->m_bordersRect;
    frontier.pop();

    // rough = true, load OSM Relation ID section is faster than honest rect-polygon intersection.
    for (auto const & cid : m_infoGetter->GetRegionsCountryIdByRect(rect, true /* rough */))
    {
      auto const nb = m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(cid));
      if (nb.IsAlive() && visited.insert(nb).second)
        if (TryAppendFromMwm(nb, osmRelID, members))
          frontier.push(nb);
    }
  }
}

bool RelationTrackBuilder::TryAppendFromMwm(MwmSet::MwmId const & mwmId, uint32_t osmRelID,
                                            std::vector<Geometry> & members)
{
  FeaturesLoaderGuard guard(m_dataSource, mwmId);
  auto const & cont = guard.GetContainer();
  if (!cont.IsExist(RELATION_OSMIDS_FILE_TAG))
    return false;

  auto const tbl = feature::FeaturesOffsetsTable::Load(cont, RELATION_OSMIDS_FILE_TAG);
  auto const idx = tbl->BinarySearch(osmRelID);
  if (!idx)
    return false;

  auto const relIdx = base::asserted_cast<uint32_t>(*idx);
  auto const nbRel = guard.GetRelation(relIdx);
  auto nbMembers = LoadMemberGeometries(nbRel, guard, RelationID(mwmId, relIdx));
  if (nbMembers.empty())
    return false;

  // Compare only with source members (members already added in this call from this
  // neighbour are themselves non-overlapping — they came from one OSM Relation).
  size_t const sz = members.size();
  for (auto & nb : nbMembers)
  {
    bool dup = false;
    for (size_t i = 0; i < sz; ++i)
      if (relation_track_merger::IsSameLine(nb, members[i]))
      {
        dup = true;
        break;
      }
    if (!dup)
      members.push_back(std::move(nb));
  }

  return true;
}

std::optional<df::TransitInfo> RelationTrackBuilder::BuildTransitInfo(uint32_t relID)
{
  FeaturesLoaderGuard guard(m_dataSource, m_fid.m_mwmId);
  auto ft = guard.GetFeatureByIndex(m_fid.m_index);
  ASSERT(ft, ());

  auto const rel = ft->ReadRelation<feature::RouteRelation>(relID);

  df::TransitInfo info;
  info.m_color = rel.GetColor();

  auto members = LoadMemberGeometries(rel, guard, RelationID(m_fid.m_mwmId, relID));
  std::vector<Geometry> lines;
  bool isSingleMWM = true;

  /// @todo Should make traversal even if |members| is empty (Subway stops only, without tunnels).
  /// But I suppuse that we will add subway tunnels geometry one day.
  if (!members.empty())
  {
    // Cross-MWM merging.
    size_t const szBefore = members.size();
    AppendNeighbourMembers(guard, relID, members);

    // Prefer Ordered algo for one only Relation.
    if (szBefore == members.size())
      lines = MergeOrdered(members);
    else
    {
      lines = MergeAllMembers(members);
      isSingleMWM = lines.empty();
    }

    info.m_routes.reserve(lines.size());
    for (auto const & line : lines)
      relation_track_merger::ToPolyline(line, info.m_routes.emplace_back().m_polyline);
  }

  // Walk stops by visiting each contributing Relation at most once across all chains.
  // Each Relation's stops are emitted in its own source order (the natural order from the OSM Relation).
  // When @p anchor is set, set highlight stop closest to the anchor (Terminal stop).
  auto const collectStops = [&](RelationID const & id, std::optional<m2::PointD> const & anchor)
  {
    if (!id.m_mwmId.IsAlive())
      return;

    auto const visit = [&](FeaturesLoaderGuard & g)
    {
      auto const r = g.GetRelation(id.m_index);
      int idx = -1;
      double nearestD = std::numeric_limits<double>::max();
      for (uint32_t const ftIdx : r.GetMembers())
      {
        auto stopFt = g.GetFeatureByIndex(ftIdx);
        if (!stopFt || stopFt->GetGeomType() != feature::GeomType::Point)
          continue;

        auto const ftCenter = stopFt->GetCenter();
        df::TransitInfo::Stop stop;
        stop.m_featureId = stopFt->GetID();
        stop.m_pos = ftCenter;
        stop.m_name = std::string(stopFt->GetReadableName());
        stop.m_highlight = (stop.m_featureId == m_fid);
        info.m_stops.push_back(std::move(stop));

        if (anchor)
        {
          /// @todo Not sure that this is the honest criteria, but I don't have a better one.
          double const d = ftCenter.SquaredLength(*anchor);
          if (d < nearestD)
          {
            nearestD = d;
            idx = info.m_stops.size() - 1;
          }
        }
      }

      if (idx >= 0)
        info.m_stops[idx].m_highlight = true;
    };

    if (id.m_mwmId == m_fid.m_mwmId)
      visit(guard);
    else
    {
      FeaturesLoaderGuard nbg(m_dataSource, id.m_mwmId);
      visit(nbg);
    }
  };

  if (!isSingleMWM)
  {
    std::unordered_set<RelationID> seenRels;
    auto const & firstID = lines.front().m_relIDs.front();
    auto const & lastID = lines.back().m_relIDs.back();

    // 1. First Relation — anchors the head; pick start terminal.
    collectStops(firstID, lines.front().FrontPoint());
    size_t const szFirst = info.m_stops.size();

    // 2. Intermediate Relations (deduped, last deferred). Order is first-visit order.
    seenRels.insert(firstID);
    seenRels.insert(lastID);
    for (auto const & line : lines)
      for (auto const & ref : line.m_relIDs)
        if (seenRels.insert(ref).second)
          collectStops(ref, std::nullopt);

    // 3. Last Relation — anchors the tail; pick end terminal.
    if (lastID != firstID)
      collectStops(lastID, lines.back().BackPoint());
    else
    {
      // Same Relation contributes to both ends — second pass over its already-emitted stops to find the end terminal.
      int idx = -1;
      double nearestD = std::numeric_limits<double>::max();
      auto const & anchor = lines.back().BackPoint();
      for (size_t i = 0; i < szFirst; ++i)
      {
        double const d = info.m_stops[i].m_pos.SquaredLength(anchor);
        if (d < nearestD)
        {
          nearestD = d;
          idx = i;
        }
      }
      if (idx >= 0)
        info.m_stops[idx].m_highlight = true;
    }
  }
  else
  {
    // No lines → fall back to the source relation's stop list.
    collectStops(RelationID(m_fid.m_mwmId, relID), std::nullopt);
    if (!info.m_stops.empty())
    {
      info.m_stops.front().m_highlight = true;
      info.m_stops.back().m_highlight = true;
    }
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
  auto members = LoadMemberGeometries(rel, guard, RelationID(m_fid.m_mwmId, relID));
  if (members.empty())
    return std::nullopt;

  auto lines = MergeOrdered(members);
  if (lines.empty())
    return std::nullopt;

  df::SelectionInfo info;
  info.m_color = rel.GetColor();
  info.m_lines.reserve(lines.size());
  for (auto const & line : lines)
    relation_track_merger::ToPolyline(line, info.m_lines.emplace_back());
  return info;
}

std::vector<RelationTrackBuilder::Geometry> RelationTrackBuilder::LoadMemberGeometries(
    feature::RouteRelation const & relation, FeaturesLoaderGuard const & guard, RelationID const & relID)
{
  auto const & ftMembers = relation.GetMembers();
  if (ftMembers.empty())
    return {};

  feature::AltitudeLoaderBase altLoader(*guard.GetHandle().GetValue());

  std::vector<Geometry> result;
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

    Geometry line;
    line.Reserve(pointsCount);
    line.Append({ft->GetPoint(0), alts[0]});
    for (size_t j = 1; j < pointsCount; ++j)
    {
      /// @todo We still have equal points in Line Feature from MWM.
      if (!relation_track_merger::IsEqual(line.BackPoint(), ft->GetPoint(j)))
        line.Append({ft->GetPoint(j), alts[j]});
    }

    if (line.Size() < 2)
      continue;

    line.AddRelationRef(relID);

    result.push_back(std::move(line));
  }
  return result;
}

RelationTrackBuilder::Geometry RelationTrackBuilder::BuildChain(std::vector<Geometry> const & members, size_t startIdx)
{
  relation_track_merger::Merger merger(members);
  return merger.BuildChain(startIdx);
}

std::vector<RelationTrackBuilder::Geometry> RelationTrackBuilder::MergeAllMembers(std::vector<Geometry> const & members)
{
  relation_track_merger::Merger merger(members);
  std::vector<Geometry> chains;

  for (size_t i = 0; i < merger.Size(); ++i)
  {
    if (merger.IsUsed(i))
      continue;

    chains.push_back(merger.BuildChain(i));
  }

  if (chains.size() <= 1)
    return chains;

  // Keep chains[0] first (preserving relation member order), but reorder the rest so
  // that each next one is closest to the previous one's end. Reverse chains as needed.

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

      auto const distFront = pt.SquaredLength(chains[j].FrontPoint());
      auto const distBack = pt.SquaredLength(chains[j].BackPoint());

      if (distFront < best.dist)
        best = {j, distFront, false};
      if (distBack < best.dist)
        best = {j, distBack, true};
    }
    return best;
  };

  std::vector<Geometry> result;
  result.reserve(chains.size());
  std::vector<bool> picked(chains.size(), false);
  picked[0] = true;

  // Check if reversing chains[0] gives a better connection to the nearest second chain.
  auto const fwd = FindNearest(chains[0].BackPoint(), picked);
  auto const rev = FindNearest(chains[0].FrontPoint(), picked);
  if (rev.dist < fwd.dist)
    chains[0].Reverse();

  result.push_back(std::move(chains[0]));

  for (size_t step = 1; step < chains.size(); ++step)
  {
    auto best = FindNearest(result.back().BackPoint(), picked);

    picked[best.idx] = true;
    if (best.reverse)
      chains[best.idx].Reverse();
    result.push_back(std::move(chains[best.idx]));
  }

  return result;
}

std::vector<RelationTrackBuilder::Geometry> RelationTrackBuilder::MergeOrdered(std::vector<Geometry> const & members)
{
  using relation_track_merger::Merger;

  std::vector<Geometry> result;
  Geometry chain = members[0];

  // Check if reversing the first member gives a better connection to the second.
  if (members.size() > 1)
  {
    bool dummy;
    bool const connectsBack = Merger::Connects(members[1], chain.BackPoint(), dummy);
    bool const connectsFront = Merger::Connects(members[1], chain.FrontPoint(), dummy);
    if (!connectsBack && connectsFront)
      chain.Reverse();
  }

  for (size_t i = 1; i < members.size(); ++i)
  {
    auto const & m = members[i];
    ASSERT_GREATER(m.Size(), 1, ());

    bool needReverse = false;
    if (Merger::Connects(m, chain.BackPoint(), needReverse))
    {
      chain.Splice(m, needReverse);
    }
    else
    {
      // Gap detected — flush current chain and start a new one.
      result.push_back(std::move(chain));
      chain = m;
    }
  }

  result.push_back(std::move(chain));
  return result;
}
