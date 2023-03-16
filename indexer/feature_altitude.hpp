#pragma once

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace feature
{
struct AltitudeHeader
{
  using TAltitudeSectionVersion = uint16_t;

  AltitudeHeader() { Reset(); }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_minAltitude);
    WriteToSink(sink, m_featureTableOffset);
    WriteToSink(sink, m_altitudesOffset);
    WriteToSink(sink, m_endOffset);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    m_version = ReadPrimitiveFromSource<TAltitudeSectionVersion>(src);
    m_minAltitude = ReadPrimitiveFromSource<geometry::Altitude>(src);
    m_featureTableOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_altitudesOffset = ReadPrimitiveFromSource<uint32_t>(src);
    m_endOffset = ReadPrimitiveFromSource<uint32_t>(src);
  }

  // Methods below return sizes of parts of altitude section in bytes.
  size_t GetAltitudeAvailabilitySize() const
  {
    return m_featureTableOffset - sizeof(AltitudeHeader);
  }

  size_t GetFeatureTableSize() const { return m_altitudesOffset - m_featureTableOffset; }

  size_t GetAltitudeInfoSize() const { return m_endOffset - m_altitudesOffset; }

  void Reset()
  {
    m_version = 0;
    m_minAltitude = geometry::kDefaultAltitudeMeters;
    m_featureTableOffset = 0;
    m_altitudesOffset = 0;
    m_endOffset = 0;
  }

  TAltitudeSectionVersion m_version;
  geometry::Altitude m_minAltitude;
  uint32_t m_featureTableOffset;
  uint32_t m_altitudesOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(AltitudeHeader) == 16, "Wrong header size of altitude section.");

class Altitudes
{
public:
  Altitudes() = default;

  explicit Altitudes(geometry::Altitudes const & altitudes) : m_altitudes(altitudes) {}

  template <class TSink>
  void Serialize(geometry::Altitude minAltitude, TSink & sink) const
  {
    CHECK(!m_altitudes.empty(), ());

    BitWriter<TSink> bits(sink);
    geometry::Altitude prevAltitude = minAltitude;
    for (auto const altitude : m_altitudes)
    {
      CHECK_LESS_OR_EQUAL(minAltitude, altitude, ());
      uint32_t const delta = bits::ZigZagEncode(static_cast<int32_t>(altitude) -
                                                static_cast<int32_t>(prevAltitude));
      // Making serialized value greater than zero.
      CHECK(coding::DeltaCoder::Encode(bits, delta + 1), ());
      prevAltitude = altitude;
    }
  }

  template <class TSource>
  void Deserialize(geometry::Altitude minAltitude, size_t pointCount,
                   std::string const & countryFileName, uint32_t featureId, TSource & src)
  {
    ASSERT_NOT_EQUAL(pointCount, 0, ());

    BitReader<TSource> bits(src);
    geometry::Altitude prevAltitude = minAltitude;
    m_altitudes.resize(pointCount);

    for (size_t i = 0; i < pointCount; ++i)
    {
      uint64_t const delta = coding::DeltaCoder::Decode(bits);
      CHECK(delta > 0, (countryFileName, featureId));

      m_altitudes[i] = static_cast<geometry::Altitude>(bits::ZigZagDecode(delta - 1) + prevAltitude);
      CHECK_LESS_OR_EQUAL(minAltitude, m_altitudes[i], (countryFileName, featureId));

      prevAltitude = m_altitudes[i];
    }
  }

  /// \note |m_altitudes| is a vector of feature point altitudes. There's two possibilities:
  /// * |m_altitudes| is empty. It means there is no altitude information for this feature.
  /// * size of |m_pointAlt| is equal to the number of this feature's points. If so
  ///   all items of |m_altitudes| have valid value.
  geometry::Altitudes m_altitudes;
};
}  // namespace feature
