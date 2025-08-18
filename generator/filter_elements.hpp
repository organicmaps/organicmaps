// This file contains the FilterData and FilterElements classes.
// With the help of them, a mechanism is implemented to skip elements from processing according to
// the rules from the configuration file.
// The configuration file is in json format.
// File example:
//  {
//    "node": {
//      "ids": [
//        1435,
//        436
//      ],
//      "tags": [
//        {
//          "place": "city"
//        },
//        {
//          "place": "town",
//          "capital": "*"
//        }
//      ]
//    },
//    "way": {},
//    "relation": {}
//  }
// This means that we will skip node element if one of the following is true:
// 1. its id equals 1435
// 2. its id equals  436
// 3. if it contains {place:city} in tags
// 4. if it contains {place:town} and {capital:*} in tags.
// '*' - means any value.
// Record format for way and relation is the same as for node.
// This implementation does not support processing of multiple values
// (https://wiki.openstreetmap.org/wiki/Multiple_values).
#pragma once

#include "generator/filter_interface.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cppjansson/cppjansson.hpp"

namespace generator
{
// This is a helper class for the FilterElements class.
// It works with identifiers and tags at a low level.
class FilterData
{
public:
  using Tags = std::vector<OsmElement::Tag>;

  void AddSkippedId(uint64_t id);
  void AddSkippedTags(Tags const & tags);
  bool NeedSkipWithId(uint64_t id) const;
  bool NeedSkipWithTags(Tags const & tags) const;

private:
  static bool IsMatch(Tags const & elementTags, Tags const & tags);

  std::unordered_set<uint64_t> m_skippedIds;
  std::unordered_multimap<std::string, std::reference_wrapper<Tags const>> m_skippedTags;
  std::list<Tags> m_rulesStorage;
};

// This is the main class that implements the element skipping mechanism.
class FilterElements : public FilterInterface
{
public:
  explicit FilterElements(std::string const & filename);

  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(OsmElement const & element) const override;

  bool NeedSkip(OsmElement const & element) const;

private:
  static bool ParseSection(json_t * json, FilterData & fdata);
  static bool ParseIds(json_t * json, FilterData & fdata);
  static bool ParseTags(json_t * json, FilterData & fdata);

  bool NeedSkip(OsmElement const & element, FilterData const & fdata) const;
  bool ParseString(std::string const & str);

  std::string m_filename;

  FilterData m_nodes;
  FilterData m_ways;
  FilterData m_relations;
};
}  // namespace generator
