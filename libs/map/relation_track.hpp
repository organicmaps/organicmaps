#pragma once

#include "drape_frontend/selection_info.hpp"

#include "kml/type_utils.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/route_relation.hpp"

#include "drape/color.hpp"

#include <optional>
#include <vector>

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

/// Builds track geometry from a route relation, starting from a specific feature
/// and growing the chain in both directions using endpoint matching.
class RelationTrackBuilder
{
public:
  /// Reference to a Relation in a particular MWM. {m_mwmId, m_index} == {MwmId, in-MWM relIdx}.
  /// FeatureID's structure matches our needs exactly so we reuse it.
  using RelationID = FeatureID;

  /// Geometry of a (possibly chained) line, plus the ordered list of source Relations
  /// that contributed runs of points to it. Way-orientation inside a Relation is
  /// arbitrary, so we record only *which* Relation each run came from — never a
  /// "reversed" flag. Consumers that need to know which end of the chain corresponds
  /// to a given Relation's first/last stop use the chain endpoint geometry, not
  /// provenance flags (see BuildTransitInfo terminal picking).
  struct Geometry
  {
    kml::TrackGeometry m_points;
    std::vector<RelationID> m_relIDs;

    bool IsEmpty() const { return m_points.empty(); }
    size_t Size() const { return m_points.size(); }

    void Reserve(size_t n) { m_points.reserve(n); }

    void Append(geometry::PointWithAltitude const & p) { m_points.push_back(p); }

    m2::PointD const & FrontPoint() const { return m_points.front().GetPoint(); }
    m2::PointD const & BackPoint() const { return m_points.back().GetPoint(); }

    /// Records that the just-appended (or about-to-be-appended) points originate from @p id.
    void AddRelationRef(RelationID const & id) { m_relIDs.push_back(id); }

    /// Inserts @p other into this Geometry, either at the back or at the front. The
    /// seam point of @p other (the one shared with this geometry's matched endpoint)
    /// is dropped, so callers don't need to peel it off themselves.
    /// @param atFront true to prepend, false to append.
    /// @param reverse iterate @p other from back to front when copying its points.
    /// Provenance from @p other travels with the points: appended (or prepended) in
    /// the same order as the points run (reversed when @p reverse is true).
    void Insert(Geometry const & other, bool atFront, bool reverse);

    /// Convenience: append @p other (atFront=false). Same semantics as Insert.
    void Splice(Geometry const & other, bool reverse) { Insert(other, false /* atFront */, reverse); }

    /// In-place reverse of both points and provenance order.
    void Reverse();
  };

  struct Data
  {
    std::vector<Geometry> m_lines;
    std::string m_name;
    dp::Color m_color;
  };

public:
  RelationTrackBuilder(DataSource const & dataSource, FeatureID const & fid,
                       storage::CountryInfoGetter const * infoGetter = nullptr);

  /// @return nullopt if no suitable relation found or geometry can't be built.
  std::optional<Data> Build();

  /// Builds a SelectionInfo (ordered polylines + color) for a specific relation (by @p relID),
  /// using MergeOrdered — suitable for public-transport routes where way ordering is meaningful.
  /// Feeds the selection-line render path (DrapeEngine::SetSelectionLines).
  std::optional<df::SelectionInfo> BuildSelectionInfo(uint32_t relID);

  /// Builds a full TransitInfo (ordered polylines + stops with names + color)
  /// from a specific relation (by @p relID). Suitable for the relation-transit render path.
  std::optional<df::TransitInfo> BuildTransitInfo(uint32_t relID);

  /// Builds the longest connected chain starting from @p startIdx, growing in both directions.
  static Geometry BuildChain(std::vector<Geometry> const & members, size_t startIdx);

  /// Merges all member geometries into connected chains using endpoint matching.
  /// Returns multiple lines when members form disconnected segments.
  static std::vector<Geometry> MergeAllMembers(std::vector<Geometry> const & members);

  /// Merges members assuming they are ordered in the relation.
  /// Consecutive members that connect are joined; gaps start new lines.
  static std::vector<Geometry> MergeOrdered(std::vector<Geometry> const & members);

private:
  /// Loads member geometries from the MWM identified by an alive @p guard.
  /// Each returned Geometry has a single RelationID seeded with @p relID.
  std::vector<Geometry> LoadMemberGeometries(feature::RouteRelation const & relation, FeaturesLoaderGuard const & guard,
                                             RelationID const & relID);

  /// Iteratively splices in this relation's geometry from neighbour MWMs (matched by
  /// OSM Relation ID via RELATION_OSMIDS_FILE_TAG). Visits each MWM at most once.
  void AppendNeighbourMembers(FeaturesLoaderGuard const & guard, uint32_t relIdx, std::vector<Geometry> & members);
  bool TryAppendFromMwm(MwmSet::MwmId const & mwmId, uint32_t osmRelID, std::vector<Geometry> & members);

  DataSource const & m_dataSource;
  FeatureID m_fid;
  storage::CountryInfoGetter const * m_infoGetter = nullptr;
};
