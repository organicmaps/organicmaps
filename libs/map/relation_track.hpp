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
  using TrackGeometry = kml::TrackGeometry;

  struct Data
  {
    std::vector<TrackGeometry> m_lines;
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
  static TrackGeometry BuildChain(std::vector<TrackGeometry> const & members, size_t startIdx);

  /// Merges all member geometries into connected chains using endpoint matching.
  /// Returns multiple lines when members form disconnected segments.
  static std::vector<TrackGeometry> MergeAllMembers(std::vector<TrackGeometry> const & members);

  /// Merges members assuming they are ordered in the relation.
  /// Consecutive members that connect are joined; gaps start new lines.
  static std::vector<TrackGeometry> MergeOrdered(std::vector<TrackGeometry> const & members);

private:
  /// Loads member geometries from the MWM identified by an alive @p handle.
  /// @param[out] startIdx index of @p m_fid.m_index inside the result, or
  ///                      numeric_limits<size_t>::max() if not present in this MWM.
  std::vector<TrackGeometry> LoadMemberGeometries(feature::RouteRelation const & relation, size_t & startIdx,
                                                  FeaturesLoaderGuard const & guard);

  /// Iteratively splices in this relation's geometry from neighbour MWMs (matched by
  /// OSM Relation ID via RELATION_OSMIDS_FILE_TAG). Visits each MWM at most once.
  void AppendNeighbourMembers(MwmSet::MwmId const & mwmId, uint32_t osmRelID, std::vector<TrackGeometry> & members);
  bool TryAppendFromMwm(MwmSet::MwmId const & mwmId, uint32_t osmRelID, std::vector<TrackGeometry> & members);

  DataSource const & m_dataSource;
  FeatureID m_fid;
  storage::CountryInfoGetter const * m_infoGetter = nullptr;
};
