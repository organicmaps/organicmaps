#pragma once

#include "geocoder/hierarchy.hpp"

#include "base/geo_object_id.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace geocoder
{
class Index
{
public:
  using EntryPtr = Hierarchy::Entry const *;

  explicit Index(Hierarchy const & hierarchy);

  // Returns a pointer to entries whose names exactly match |tokens|
  // (the order matters) or nullptr if there are no such entries.
  //
  // todo This method (and the whole class, in fact) is in the
  //      prototype stage and may be too slow. Proper indexing should
  //      be implemented to perform this type of queries.
  std::vector<EntryPtr> const * const GetEntries(Tokens const & tokens) const;

  std::vector<EntryPtr> const * const GetBuildingsOnStreet(base::GeoObjectId const & osmId) const;

private:
  // Adds address information of entries to the index.
  void AddEntries();

  // Adds the street |e| to the index, with and without synonyms
  // of the word "street".
  void AddStreet(Hierarchy::Entry const & e);

  // Fills the |m_buildingsOnStreet| field.
  void AddHouses();

  std::vector<Hierarchy::Entry> const & m_entries;

  std::unordered_map<std::string, std::vector<EntryPtr>> m_entriesByTokens;

  // Lists of houses grouped by the streets they belong to.
  std::unordered_map<base::GeoObjectId, std::vector<EntryPtr>> m_buildingsOnStreet;
};
}  // namespace geocoder
