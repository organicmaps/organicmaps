#pragma once
#include "routing/restrictions_serialization.hpp"

#include "base/osm_id.hpp"

#include <functional>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
/// This class collects all relations with type restriction and save feature ids of
/// their road feature in text file for using later.
class RestrictionCollector
{
public:
  RestrictionCollector() = default;
  /// \param restrictionPath full path to file with road restrictions in osm id terms.
  /// \param osmIdsToFeatureIdsPath full path to file with mapping from osm ids to feature ids.
  RestrictionCollector(std::string const & restrictionPath, std::string const & osmIdsToFeatureIdsPath);

  bool HasRestrictions() const { return !m_restrictions.empty(); }

  /// \returns true if all restrictions in |m_restrictions| are valid and false otherwise.
  /// \note Complexity of the method is linear in the size of |m_restrictions|.
  bool IsValid() const;

  /// \returns Sorted vector of restrictions.
  RestrictionVec const & GetRestrictions() const { return m_restrictions; }

private:
  friend void UnitTest_RestrictionTest_ValidCase();
  friend void UnitTest_RestrictionTest_InvalidCase();
  friend void UnitTest_RestrictionTest_ParseRestrictions();
  friend void UnitTest_RestrictionTest_ParseFeatureId2OsmIdsMapping();

  /// \brief Parses comma separated text file with line in following format:
  /// <type of restrictions>, <osm id 1 of the restriction>, <osm id 2>, and so on
  /// For example:
  /// Only, 335049632, 49356687,
  /// No, 157616940, 157616940,
  /// No, 157616940, 157617107,
  /// \param path path to the text file with restrictions.
  bool ParseRestrictions(std::string const & path);

  /// \brief Adds feature id and corresponding |osmId| to |m_osmIdToFeatureId|.
  void AddFeatureId(uint32_t featureId, osm::Id osmId);

  /// \brief Adds a restriction (vector of osm id).
  /// \param type is a type of restriction
  /// \param osmIds is osm ids of restriction links
  /// \note This method should be called to add a restriction when feature ids of the restriction
  /// are unknown. The feature ids should be set later with a call of |SetFeatureId(...)| method.
  /// \returns true if restriction is add and false otherwise.
  bool AddRestriction(Restriction::Type type, std::vector<osm::Id> const & osmIds);

  RestrictionVec m_restrictions;
  std::map<osm::Id, uint32_t> m_osmIdToFeatureId;
};

bool FromString(std::string str, Restriction::Type & type);
}  // namespace routing
