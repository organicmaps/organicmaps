#pragma once

#include "generator/restriction_writer.hpp"
#include "generator/routing_helpers.hpp"
#include "generator/routing_index_generator.hpp"

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/restrictions_serialization.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace routing_builder
{
class TestRestrictionCollector;
using Restriction = routing::Restriction;

/// This class collects all relations with type restriction and save feature ids of
/// their road feature in text file for using later.
class RestrictionCollector
{
public:
  static m2::PointD constexpr kNoCoords =
      m2::PointD(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

  RestrictionCollector(std::string const & osmIdsToFeatureIdPath, routing::IndexGraph & graph);

  bool Process(std::string const & restrictionPath);

  bool HasRestrictions() const { return !m_restrictions.empty(); }

  /// \returns Sorted vector of restrictions.
  std::vector<Restriction> && StealRestrictions() { return std::move(m_restrictions); }

private:
  friend class TestRestrictionCollector;

  /// \brief Parses comma separated text file with line in following format:
  /// In case of restriction, that consists of "way(from)" -> "node(via)" -> "way(to)"
  /// RType, VType, X, Y, f1, f2
  /// Where RType = "No" or "Only",
  ///       VType = "node",
  ///       f1, f2 - "from" and "to" features id,
  ///       X, Y - coords of node.
  ///              Needs to check, that f1 and f2 intersect at this point.
  ///
  /// In case of restriction, that consists of several ways as via members:
  ///   "way(from)" -> "way(via)" -> ... -> "way(via)" -> "way(to)", next format:
  /// RType, VType, f1, f2, f3, ..., fN
  /// Where RType = "No" or "Only",
  ///       VType = "way",
  ///       f1, f2, ..., fN - "from", "via", ..., "via", "to" members.
  /// For each neighboring features we check that they have common point.
  /// \param path path to the text file with restrictions.
  bool ParseRestrictions(std::string const & path);

  /// \brief Adds feature id and corresponding |osmId| to |m_osmIdToFeatureId|.
  void AddFeatureId(uint32_t featureId, base::GeoObjectId osmId);

  /// \brief In case of |coords| not equal kNoCoords, searches point at |prev| with
  /// coords equals to |coords| and checks that the |cur| is outgoes from |prev| at
  /// this point.
  /// In case of |coords| equals to kNoCoords, just checks, that |prev| and |cur| has common
  /// junctions.
  bool FeaturesAreCross(m2::PointD const & coords, uint32_t prev, uint32_t cur) const;

  bool IsRestrictionValid(Restriction::Type & restrictionType, m2::PointD const & coords,
                          std::vector<uint32_t> & featureIds) const;

  bool CheckAndProcessUTurn(Restriction::Type & restrictionType, m2::PointD const & coords,
                            std::vector<uint32_t> & featureIds) const;

  routing::Joint::Id GetFirstCommonJoint(uint32_t firstFeatureId, uint32_t secondFeatureId) const;

  bool FeatureHasPointWithCoords(uint32_t featureId, m2::PointD const & coords) const;
  /// \brief Adds a restriction (vector of osm id).
  /// \param type is a type of restriction
  /// \param osmIds is osm ids of restriction links
  /// \note This method should be called to add a restriction when feature ids of the restriction
  /// are unknown. The feature ids should be set later with a call of |SetFeatureId(...)| method.
  /// \returns true if restriction is add and false otherwise.
  bool AddRestriction(m2::PointD const & coords, Restriction::Type type, std::vector<base::GeoObjectId> const & osmIds);

  std::vector<Restriction> m_restrictions;
  routing::OsmIdToFeatureIds m_osmIdToFeatureIds;

  routing::IndexGraph & m_indexGraph;

  std::string m_restrictionPath;
};

void FromString(std::string_view str, Restriction::Type & type);
void FromString(std::string_view str, RestrictionWriter::ViaType & type);
void FromString(std::string_view str, double & number);
}  // namespace routing_builder
