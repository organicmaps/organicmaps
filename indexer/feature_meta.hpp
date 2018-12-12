#pragma once

#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>


namespace feature
{
class MetadataBase
{
protected:
  // TODO: Change uint8_t to appropriate type when FMD_COUNT reaches 256.
  void Set(uint8_t type, std::string const & value)
  {
    auto found = m_metadata.find(type);
    if (found == m_metadata.end())
    {
      if (!value.empty())
        m_metadata[type] = value;
    }
    else
    {
      if (value.empty())
        m_metadata.erase(found);
      else
        found->second = value;
    }
  }

public:
  bool Has(uint8_t type) const
  {
    auto const it = m_metadata.find(type);
    return it != m_metadata.end();
  }

  std::string Get(uint8_t type) const
  {
    auto const it = m_metadata.find(type);
    return (it == m_metadata.end()) ? std::string() : it->second;
  }

  std::vector<uint8_t> GetPresentTypes() const
  {
    std::vector<uint8_t> types;
    types.reserve(m_metadata.size());

    for (auto const & item : m_metadata)
      types.push_back(item.first);

    return types;
  }

  inline bool Empty() const { return m_metadata.empty(); }
  inline size_t Size() const { return m_metadata.size(); }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    auto const sz = static_cast<uint32_t>(m_metadata.size());
    WriteVarUint(sink, sz);
    for (auto const & it : m_metadata)
    {
      WriteVarUint(sink, static_cast<uint32_t>(it.first));
      utils::WriteString(sink, it.second);
    }
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    auto const sz = ReadVarUint<uint32_t>(src);
    for (size_t i = 0; i < sz; ++i)
    {
      auto const key = ReadVarUint<uint32_t>(src);
      utils::ReadString(src, m_metadata[key]);
    }
  }

  inline bool Equals(MetadataBase const & other) const
  {
    return m_metadata == other.m_metadata;
  }

protected:
  std::map<uint8_t, std::string> m_metadata;
};

class Metadata : public MetadataBase
{
public:
  /// @note! Do not change values here.
  /// Add new types to the end of list, before FMD_COUNT.
  /// For types parsed from OSM get corresponding OSM tag to MetadataTagProcessor::TypeFromString().
  enum EType : int8_t
  {
    FMD_CUISINE = 1,
    FMD_OPEN_HOURS = 2,
    FMD_PHONE_NUMBER = 3,
    FMD_FAX_NUMBER = 4,
    FMD_STARS = 5,
    FMD_OPERATOR = 6,
    FMD_URL = 7,
    FMD_WEBSITE = 8,
    FMD_INTERNET = 9,
    FMD_ELE = 10,
    FMD_TURN_LANES = 11,
    FMD_TURN_LANES_FORWARD = 12,
    FMD_TURN_LANES_BACKWARD = 13,
    FMD_EMAIL = 14,
    FMD_POSTCODE = 15,
    FMD_WIKIPEDIA = 16,
    // FMD_MAXSPEED used to be 17 but now it is stored in a section of its own.
    FMD_FLATS = 18,
    FMD_HEIGHT = 19,
    FMD_MIN_HEIGHT = 20,
    FMD_DENOMINATION = 21,
    FMD_BUILDING_LEVELS = 22,
    FMD_TEST_ID = 23,
    FMD_SPONSORED_ID = 24,
    FMD_PRICE_RATE = 25,
    FMD_RATING = 26,
    FMD_BANNER_URL = 27,
    FMD_LEVEL = 28,
    FMD_AIRPORT_IATA = 29,
    FMD_BRAND = 30,
    FMD_COUNT
  };

  /// Used to normalize tags like "contact:phone" and "phone" to a common metadata enum value.
  static bool TypeFromString(std::string const & osmTagKey, EType & outType);
  static bool IsSponsoredType(EType const & type);

  void Set(EType type, std::string const & value) { MetadataBase::Set(type, value); }
  void Drop(EType type) { Set(type, std::string()); }
  std::string GetWikiURL() const;

  // TODO: Commented code below is now longer neded, but I leave it here
  // as a hint to what is going on in DeserializeFromMWMv7OrLower.
  // Please, remove it when DeserializeFromMWMv7OrLower is no longer neded.
  // template <class TWriter>
  // void SerializeToMWM(TWriter & writer) const
  // {
  //   for (auto const & e : m_metadata)
  //   {
  //     // Set high bit if it's the last element.
  //     uint8_t const mark = (&e == &(*m_metadata.crbegin()) ? 0x80 : 0);
  //     uint8_t elem[2] = {static_cast<uint8_t>(e.first | mark),
  //                        static_cast<uint8_t>(min(e.second.size(), (size_t)kMaxStringLength))};
  //     writer.Write(elem, sizeof(elem));
  //     writer.Write(e.second.data(), elem[1]);
  //   }
  // }

  template <class TSource>
  void DeserializeFromMWMv7OrLower(TSource & src)
  {
    uint8_t header[2] = {0};
    char buffer[kMaxStringLength] = {0};
    do
    {
      src.Read(header, sizeof(header));
      src.Read(buffer, header[1]);
      m_metadata[header[0] & 0x7F].assign(buffer, header[1]);
    } while (!(header[0] & 0x80));
  }

private:
  enum { kMaxStringLength = 255 };
};

class AddressData : public MetadataBase
{
public:
  enum Type { PLACE, STREET, POSTCODE };

  void Add(Type type, std::string const & s)
  {
    /// @todo Probably, we need to add separator here and store multiple values.
    MetadataBase::Set(type, s);
  }
};

class RegionData : public MetadataBase
{
public:
  enum Type : int8_t
  {
    RD_LANGUAGES,        // list of written languages
    RD_DRIVING,          // left- or right-hand driving (letter 'l' or 'r')
    RD_TIMEZONE,         // UTC timezone offset, floating signed number of hours: -3, 4.5
    RD_ADDRESS_FORMAT,   // address format, re: mapzen
    RD_PHONE_FORMAT,     // list of strings in "+N NNN NN-NN-NN" format
    RD_POSTCODE_FORMAT,  // list of strings in "AAA ANN" format
    RD_PUBLIC_HOLIDAYS,  // fixed PH dates
    RD_ALLOW_HOUSENAMES  // 'y' if housenames are commonly used
  };

  // Special values for month references in public holiday definitions.
  enum PHReference : int8_t
  {
    PH_EASTER = 20,
    PH_ORTHODOX_EASTER = 21,
    PH_VICTORIA_DAY = 22,
    PH_CANADA_DAY = 23
  };

  void Set(Type type, std::string const & s)
  {
    CHECK_NOT_EQUAL(type, Type::RD_LANGUAGES, ("Please use RegionData::SetLanguages method"));
    MetadataBase::Set(type, s);
  }

  void SetLanguages(std::vector<std::string> const & codes);
  void GetLanguages(std::vector<int8_t> & langs) const;
  bool HasLanguage(int8_t const lang) const;
  bool IsSingleLanguage(int8_t const lang) const;

  void AddPublicHoliday(int8_t month, int8_t offset);
  // No public holidays getters until we know what to do with these.
};
}  // namespace feature

// Prints types in osm-friendly format.
std::string ToString(feature::Metadata::EType type);
inline std::string DebugPrint(feature::Metadata::EType type) { return ToString(type); }
