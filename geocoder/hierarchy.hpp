#pragma once

#include "geocoder/name_dictionary.hpp"
#include "geocoder/types.hpp"

#include "base/geo_object_id.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
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

    // Number of base::GeoObjectsIds that occur as keys in at least two entries.
    uint64_t m_duplicateOsmIds = 0;

    // Number of entries with duplicate subfields in the address field.
    uint64_t m_duplicateAddresses = 0;

    // Number of entries whose address field either does
    // not exist or consists of empty lines.
    uint64_t m_emptyAddresses = 0;

    // Number of entries without the name field or with an empty one.
    uint64_t m_emptyNames = 0;

    // Number of street entries without a locality name.
    uint64_t m_noLocalityStreets = 0;

    // Number of building entries without a locality name.
    uint64_t m_noLocalityBuildings = 0;

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
    bool DeserializeFromJSON(std::string const & jsonStr,
                             NameDictionaryMaker & normalizedNameDictionaryMaker,
                             ParsingStats & stats);
    bool DeserializeFromJSONImpl(json_t * const root, std::string const & jsonStr,
                                 NameDictionaryMaker & normalizedNameDictionaryMaker,
                                 ParsingStats & stats);

    std::string const & GetNormalizedName(Type type,
                                          NameDictionary const & normalizedNameDictionary) const;
    bool operator<(Entry const & rhs) const { return m_osmId < rhs.m_osmId; }

    base::GeoObjectId m_osmId = base::GeoObjectId(base::GeoObjectId::kInvalid);

    // Original name of the entry. Useful for debugging.
    std::string m_name;

    Type m_type = Type::Count;

    // The positions of entry address fields in normalized name dictionary, one per Type.
    std::array<NameDictionary::Position, static_cast<size_t>(Type::Count)> m_normalizedAddress{};
  };

  explicit Hierarchy(std::vector<Entry> && entries, NameDictionary && normalizeNameDictionary);

  std::vector<Entry> const & GetEntries() const;
  NameDictionary const & GetNormalizedNameDictionary() const;

  Entry const * GetEntryForOsmId(base::GeoObjectId const & osmId) const;
  bool IsParentTo(Hierarchy::Entry const & entry, Hierarchy::Entry const & toEntry) const;

private:
  std::vector<Entry> m_entries;
  NameDictionary m_normalizedNameDictionary;
};
}  // namespace geocoder
