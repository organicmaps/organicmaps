#pragma once

#include "geocoder/types.hpp"

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
class Hierarchy
{
public:
  // A single entry in the hierarchy directed acyclic graph.
  // Currently, this is more or less the "properties"-"address"
  // part of the geojson entry.
  struct Entry
  {
    bool DeserializeFromJSON(std::string const & jsonStr);

    void DeserializeFromJSONImpl(json_t * const root, std::string const & jsonStr);

    // Tries to set |m_name| and |m_nameTokens| from
    // the "name" and "address" fields in the json description.
    void SetName(json_t * const properties, std::string const & jsonStr);

    osm::Id m_osmId = osm::Id(osm::Id::kInvalid);

    // Original name of the entry. Useful for debugging.
    std::string m_name;
    // Tokenized and simplified name of the entry.
    std::vector<strings::UniString> m_nameTokens;

    Type m_type = Type::Count;

    // The address fields of this entry, one per Type.
    std::array<Tokens, static_cast<size_t>(Type::Count) + 1> m_address;
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
}  // namespace geocoder
