#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

namespace base
{
// GeoObjectId is used to pack the source of a geographical object together with its
// id within this source (that is, its serial number) into a single 64-bit number.
//
// The bit encoding is as follows:
//   0sss ssss RRRR RRRR xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
//   R - reserved bits
//   s - bits for object source
//   x - bits for serial number (object id within its source)
//
// The highest byte encodes one of (2^7 - 1) = 127 possible object sources.
// Another byte is reserved and the last 6 bytes leave us with 2^48 possible values that can be
// used for ids within a source.
// Typically, the reserved byte will be zero but it may be used in future if the format changes.
// At the time of writing (August 2018), OSM has approximately 2^32 different nodes with ids
// starting from one (https://wiki.openstreetmap.org/wiki/Stats) and this is by far the largest
// serial numbers that we use.
// The highest bit is zero so that the resulting number is positive if read as a signed 64-bit
// integer in two's complement notation. This is important for readability in some database systems
// that do not support unsigned integers. An older scheme we used to store OsmIds tried to keep as
// much bits for the serial number as possible and utilized the highest bit which resulted in a lot
// of "negative" numbers.
//
// When all bits are set to zero the GeoObjectId is defined to be invalid.
//
// Another way would be to use separate headers for source and for categories within the source,
// as in OSM->Way->Id instead of OSMWay->Id that we have now but we do not have this many sources
// and the difference does not seem important. Also this would probably touch the highest bit.
class GeoObjectId
{
public:
  // Sources of the objects.
  enum class Type : uint8_t
  {
    Invalid = 0x00,
    OsmNode = 0x01,
    OsmWay = 0x02,
    OsmRelation = 0x03,
    BookingComNode = 0x04,

    // Artificial objects that substitute objects not presented in OSM for some reason.
    // E.g., streets that only exist in addr:street fields of houses but not as separate OSM ways.
    OsmSurrogate = 0x05,

    // Federal informational address system. http://www.ifias.ru/
    Fias = 0x06,

    ObsoleteOsmNode = 0x40,
    ObsoleteOsmWay = 0x80,
    ObsoleteOsmRelation = 0xC0,
  };

  static constexpr uint64_t kInvalid = 0ULL;

  explicit GeoObjectId(uint64_t encodedId = kInvalid);
  explicit GeoObjectId(Type type, uint64_t id);

  // Returns the id that the object has within its source.
  uint64_t GetSerialId() const;

  // Returns the encoded value that contains both
  // the source type and the serial number.
  uint64_t GetEncodedId() const;

  // Returns the source type of the object.
  Type GetType() const;

  bool operator<(GeoObjectId const & other) const { return m_encodedId < other.m_encodedId; }
  bool operator==(GeoObjectId const & other) const { return m_encodedId == other.m_encodedId; }
  bool operator!=(GeoObjectId const & other) const { return !(*this == other); }

private:
  uint64_t m_encodedId;
};

std::ostream & operator<<(std::ostream & os, GeoObjectId const & geoObjectId);
std::istream & operator>>(std::istream & os, GeoObjectId & geoObjectId);

// Helper functions for readability.
GeoObjectId MakeOsmNode(uint64_t id);
GeoObjectId MakeOsmWay(uint64_t id);
GeoObjectId MakeOsmRelation(uint64_t id);

std::string DebugPrint(GeoObjectId::Type const & t);
std::string DebugPrint(GeoObjectId const & id);
}  // namespace base

namespace std
{
template <>
struct hash<base::GeoObjectId>
{
  std::size_t operator()(base::GeoObjectId const & k) const
  {
    auto const hash = std::hash<uint64_t>();
    return hash(k.GetEncodedId());
  }
};
}  // namespace std
