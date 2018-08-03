#pragma once

#include "geocoder/types.hpp"

#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
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
  struct ParsingStats
  {
    // Number of entries that the hierarchy was constructed from.
    uint64_t m_numLoaded = 0;

    // Number of corrupted json lines.
    uint64_t m_badJsons = 0;

    // Number of entries with unreadable base::GeoObjectIds.
    uint64_t m_badOsmIds = 0;

    // Number of entries with duplicate subfields in the address field.
    uint64_t m_duplicateAddresses = 0;

    // Number of entries whose address field either does
    // not exist or consists of empty lines.
    uint64_t m_emptyAddresses = 0;

    // Number of entries without the name field or with an empty one.
    uint64_t m_emptyNames = 0;

    // Number of entries whose names do not match the most
    // specific parts of their addresses.
    // This is expected from POIs but not from regions or streets.
    uint64_t m_mismatchedNames = 0;
  };

  // A single entry in the hierarchy directed acyclic graph.
  // Currently, this is more or less the "properties"-"address"
  // part of the geojson entry.
  struct Entry
  {
    bool DeserializeFromJSON(std::string const & jsonStr, ParsingStats & stats);

    void DeserializeFromJSONImpl(json_t * const root, std::string const & jsonStr,
                                 ParsingStats & stats);

    base::GeoObjectId m_osmId = base::GeoObjectId(base::GeoObjectId::kInvalid);

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
