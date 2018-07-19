#pragma once

#include "base/osm_id.hpp"
#include "base/string_utils.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "3party/jansson/myjansson.hpp"

namespace geocoder
{
using Tokens = std::vector<strings::UniString>;

class Hierarchy
{
public:
  enum class EntryType
  {
    // It is important that the types are ordered from
    // the more general to the more specific.
    Country,
    Region,
    Subregion,
    Locality,
    Sublocality,
    Suburb,
    Building,

    Count
  };

  // A single entry in the hierarchy directed acyclic graph.
  // Currently, this is more or less the "properties"-"address"
  // part of the geojson entry.
  struct Entry
  {
    bool DeserializeFromJSON(std::string const & jsonStr);

    void DeserializeFromJSONImpl(json_t * root);

    osm::Id m_osmId = osm::Id(osm::Id::kInvalid);
    std::string m_name;
    std::vector<strings::UniString> m_nameTokens;

    EntryType m_type = EntryType::Count;

    // The address fields of this entry, one per EntryType.
    std::array<Tokens, static_cast<size_t>(EntryType::Count) + 1> m_address;
  };

  explicit Hierarchy(std::string const & pathToJsonHierarchy);

  // Returns a pointer to entries whose names exactly match |tokens|
  // (the order matters) or nullptr if there are no such entries.
  //
  // todo This method (and the whole class, in fact) is in the
  //      prototype stage and may be too slow. Proper indexing should
  //      be implemented to perform this type of queries.a
  std::vector<Entry> const * const GetEntries(std::vector<strings::UniString> const & tokens) const;

private:
  std::map<Tokens, std::vector<Entry>> m_entries;
};

std::string DebugPrint(Hierarchy::EntryType const & type);
}  // namespace geocoder
