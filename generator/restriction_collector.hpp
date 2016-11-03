#pragma once

#include "indexer/routing.hpp"

#include "std/functional.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
/// This class collects all relations with type restriction and save feature ids of
/// their road feature in text file for using later.
class RestrictionCollector
{
  friend void UnitTest_RestrictionTest_ValidCase();
  friend void UnitTest_RestrictionTest_InvalidCase();
  friend void UnitTest_RestrictionTest_ParseRestrictions();
  friend void UnitTest_RestrictionTest_ParseFeatureId2OsmIdsMapping();

public:
  /// \brief Addresses a link in vector<Restriction>.
  struct Index
  {
    size_t m_restrictionNumber; // Restriction number in restriction vector.
    size_t m_linkNumber; // Link number for a restriction. It's equal to zero or one for most cases.

    bool operator==(Index const & index) const
    {
      return m_restrictionNumber == index.m_restrictionNumber
          && m_linkNumber == index.m_linkNumber;
    }
  };

  /// \param restrictionPath full path to file with road restrictions in osm id terms.
  /// \param featureId2OsmIdsPath full path to file with mapping from feature id to osm id.
  RestrictionCollector(string const & restrictionPath, string const & featureId2OsmIdsPath);

  /// \returns true if |m_restrictions| is not empty all feature ids in |m_restrictions|
  /// are set to valid value and false otherwise.
  /// \note Empty |m_restrictions| is considered as an invalid restriction.
  /// \note Complexity of the method is up to linear in the size of |m_restrictions|.
  bool IsValid() const;

  /// \returns Sorted vector of restrictions.
  RestrictionVec const & GetRestrictions() const { return m_restrictions; }

private:
  /// \brief Parses comma separated text file with line in following format:
  /// <feature id>, <osm id 1 corresponding feature id>, <osm id 2 corresponding feature id>, and so on
  /// For example:
  /// 137999, 5170186,
  /// 138000, 5170209,
  /// 138001, 5170228,
  /// \param featureId2OsmIdsPath path to the text file.
  /// \note Most restrictions consist of type and two linear(road) features.
  /// \note For the time being only line-point-line restritions are supported.
  bool ParseFeatureId2OsmIdsMapping(string const & featureId2OsmIdsPath);

  /// \brief Parses comma separated text file with line in following format:
  /// <type of restrictions>, <osm id 1 of the restriction>, <osm id 2>, and so on
  /// For example:
  /// Only, 335049632, 49356687,
  /// No, 157616940, 157616940,
  /// No, 157616940, 157617107,
  /// \param featureId2OsmIdsPath path to the text file.
  bool ParseRestrictions(string const & restrictionPath);

  /// \brief Sets feature id for all restrictions in |m_restrictions|.
  void ComposeRestrictions();

  /// \brief removes all restriction with incorrect feature id.
  /// \note The method should be called after ComposeRestrictions().
  void RemoveInvalidRestrictions();

  /// \brief Adds feature id and corresponding vector of |osmIds| to |m_osmId2FeatureId|.
  /// \note One feature id (|featureId|) may correspond to several osm ids (|osmIds|).
  void AddFeatureId(Restriction::FeatureId featureId, vector<uint64_t> const & osmIds);

  /// \brief Adds a restriction (vector of osm id).
  /// \param type is a type of restriction
  /// \param osmIds is osm ids of restriction links
  /// \note This method should be called to add a restriction when feature ids of the restriction
  /// are unknown. The feature ids should be set later with a call of |SetFeatureId(...)| method.
  void AddRestriction(Restriction::Type type, vector<uint64_t> const & osmIds);

  RestrictionVec m_restrictions;
  vector<pair<uint64_t, Index>> m_restrictionIndex;

  unordered_multimap<uint64_t, Restriction::FeatureId> m_osmIds2FeatureId;
};

string ToString(Restriction::Type const & type);
bool FromString(string str, Restriction::Type & type);
string DebugPrint(Restriction::Type const & type);
string DebugPrint(RestrictionCollector::Index const & index);
string DebugPrint(Restriction const & restriction);
}  // namespace routing
