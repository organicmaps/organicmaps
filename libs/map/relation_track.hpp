#pragma once

#include "drape_frontend/transit_info.hpp"

#include "kml/type_utils.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/route_relation.hpp"

#include "drape/color.hpp"

#include <optional>
#include <vector>

/// Builds track geometry from a route relation, starting from a specific feature
/// and growing the chain in both directions using endpoint matching.
class RelationTrackBuilder
{
public:
  using TrackGeometry = kml::TrackGeometry;

  struct Data
  {
    std::vector<TrackGeometry> m_lines;
    std::string m_name;
    dp::Color m_color;
  };

public:
  RelationTrackBuilder(DataSource const & dataSource, FeatureID const & fid);

  /// @return nullopt if no suitable relation found or geometry can't be built.
  std::optional<Data> Build();

  /// Builds geometry for a specific relation (by @p relID) assuming members are ordered.
  /// Uses MergeOrdered — suitable for public-transport routes where way ordering is meaningful.
  std::optional<Data> BuildOrdered(uint32_t relID);

  /// Builds a full TransitInfo (ordered polylines + stops with names + color/title)
  /// from a specific relation (by @p relID). Suitable for the relation-transit render path.
  std::optional<df::TransitInfo> BuildTransitInfo(uint32_t relID);

  /// Builds the longest connected chain starting from @p startIdx, growing in both directions.
  static TrackGeometry BuildChain(std::vector<TrackGeometry> const & members, size_t startIdx);

  /// Merges all member geometries into connected chains using endpoint matching.
  /// Returns multiple lines when members form disconnected segments.
  static std::vector<TrackGeometry> MergeAllMembers(std::vector<TrackGeometry> const & members);

  /// Merges members assuming they are ordered in the relation.
  /// Consecutive members that connect are joined; gaps start new lines.
  static std::vector<TrackGeometry> MergeOrdered(std::vector<TrackGeometry> const & members);

private:
  std::vector<TrackGeometry> LoadMemberGeometries(feature::RouteRelation const & relation, size_t & startIdx);

  DataSource const & m_dataSource;
  FeatureID m_fid;
};
