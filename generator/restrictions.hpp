#pragma once

#include "generator/osm_id.hpp"

#include "std/limits.hpp"
#include "std/mutex.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class RelationElement;

namespace std
{
  template <> struct hash<osm::Id>
  {
    size_t operator()(osm::Id const & id) const
    {
      return hash<double>()(id.OsmId());
    }
  };
}  // namespace std

/// This class collects all relations with type restriction and save feature ids of
/// their road feature in text file for using later.
class RestrictionCollector
{
  friend void UnitTest_RestrictionTest_ValidCase();
  friend void UnitTest_RestrictionTest_InvalidCase();
public:
  using FeatureId = uint64_t;
  static FeatureId const kInvalidFeatureId;

  ~RestrictionCollector();

  /// \brief Types of road graph restrictions.
  /// \note Despite the fact more that 10 restriction tags are present in osm all of them
  /// could be split into two categories.
  /// * no_left_turn, no_right_turn, no_u_turn and so on go to "No" category.
  /// * only_left_turn, only_right_turn and so on go to "Only" category.
  /// That's enough to rememeber if
  /// * there's only way to pass the junction is driving along the restriction (Only)
  /// * driving along the restriction is prohibited (No)
  enum class Type
  {
    No, // Going according such restriction is prohibited.
    Only, // Only going according such restriction is permitted
  };

  /// \brief Restriction to modify road graph.
  struct Restriction
  {
    Restriction(Type type, size_t linkNumber);
    // Constructor for testing.
    Restriction(Type type, vector<FeatureId> const & links);

    bool IsValid() const;
    bool operator==(Restriction const & restriction) const;
    bool operator<(Restriction const & restriction) const;

    vector<FeatureId> m_links;
    Type m_type;
  };

  /// \brief Addresses a link in vector<Restriction>.
  struct Index
  {
    size_t m_restrictionNumber; // Restriction number in restriction vector.
    size_t m_linkNumber; // Link number for a restriction. It's equal to zero or one for most cases.
  };

  /// \brief Coverts |relationElement| to Restriction and adds it to |m_restrictions| and
  /// |m_restrictionIndex| if |relationElement| is a restriction.
  /// \note For the time being only line-point-line restritions are processed. The other restrictions
  /// are ignored.
  // @TODO(bykoianko) It's necessary to process all kind of restrictions.
  void AddRestriction(RelationElement const & relationElement);

  /// \brief Adds feature id and corresponding vector of |osmIds| to |m_osmId2FeatureId|.
  /// \note One feature id (|featureId|) may correspond to several osm ids (|osmIds|).
  void AddFeatureId(vector<osm::Id> const & osmIds, FeatureId featureId);

  /// \brief Save all restrictions (content of |m_restrictions|) to text file in the following format:
  /// * One restriction is saved in one line
  /// * Line format is: <type of restriction>, <feature id of the first feature>, <feature id of the next feature>, ...
  /// * Example 1: "No, 12345, 12346"
  /// * Example 2: "Only, 12349, 12341"
  /// \note Most restrictions consist of type and two linear(road) features.
  /// \note For the time being only line-point-line restritions are supported.
  void ComposeRestrictionsAndSave(string const & fullPath);

private:
  /// \returns true if all feature ids in |m_restrictions| are set to valid value
  /// and false otherwise.
  /// \note The method may be called after ComposeRestrictions().
  bool CheckCorrectness() const;

  /// \brief removes all restriction with incorrect feature id.
  /// \note The method should be called after ComposeRestrictions().
  void RemoveInvalidRestrictions();

  /// \brief Sets feature id for all restrictions in |m_restrictions|.
  void ComposeRestrictions();

  /// \brief Adds a restriction (vector of osm::Id).
  /// \param links is osm ids of restriction links
  /// \param type is a type of restriction
  /// \note This method should be called to add a restriction when feature ids of the restriction
  /// are unknown. The feature ids should be set later with a call of |SetFeatureId(...)| method.
  void AddRestriction(vector<osm::Id> const & links, Type type);

  mutex m_mutex;
  vector<Restriction> m_restrictions;
  vector<pair<osm::Id, Index>> m_restrictionIndex;

  unordered_multimap<osm::Id, FeatureId> m_osmIds2FeatureId;
};

string DebugPrint(RestrictionCollector::Type const & type);
string DebugPrint(RestrictionCollector::Restriction const & restriction);
