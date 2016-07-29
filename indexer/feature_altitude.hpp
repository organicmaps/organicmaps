#pragma once
#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/vector.hpp"

namespace feature
{
using TAltitude = int16_t;
using TAltitudes = vector<feature::TAltitude>;

TAltitude constexpr kInvalidAltitude = numeric_limits<TAltitude>::min();
feature::TAltitude constexpr kDefaultAltitudeMeters = 0;

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
    m_minAltitude = ReadPrimitiveFromSource<TAltitude>(src);
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
    m_minAltitude = kDefaultAltitudeMeters;
    m_featureTableOffset = 0;
    m_altitudesOffset = 0;
    m_endOffset = 0;
  }

  TAltitudeSectionVersion m_version;
  TAltitude m_minAltitude;
  uint32_t m_featureTableOffset;
  uint32_t m_altitudesOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(AltitudeHeader) == 16, "Wrong header size of altitude section.");

class Altitudes
{
public:
  Altitudes() = default;

  explicit Altitudes(TAltitudes const & altitudes) : m_altitudes(altitudes) {}

  template <class TSink>
  void Serialize(TAltitude minAltitude, TSink & sink) const
  {
    vector<uint32_t> deviations;
    PrepareSerializationData(minAltitude, deviations);

    BitWriter<TSink> bits(sink);
    for (auto const d : deviations)
      coding::DeltaCoder::Encode(bits, d);
  }

  template <class TSource>
  bool Deserialize(TAltitude minAltitude, size_t pointCount, TSource & src)
  {
    vector<uint32_t> deviations(pointCount);
    BitReader<TSource> bits(src);

    for (size_t i = 0; i < pointCount; ++i)
      deviations[i] = coding::DeltaCoder::Decode(bits);

    return FillAltitudesByDeserializedDate(minAltitude, deviations);
  }

  /// \note |m_altitudes| is a vector of feature point altitudes. There's two possibilities:
  /// * |m_altitudes| is empty. It means there is no altitude information for this feature.
  /// * size of |m_pointAlt| is equal to the number of this feature's points. If so
  ///   all items of |m_altitudes| have valid value.
  TAltitudes m_altitudes;

private:
  void PrepareSerializationData(TAltitude minAltitude, vector<uint32_t> & deviations) const;
  bool FillAltitudesByDeserializedDate(TAltitude minAltitude, vector<uint32_t> const & deviations);
};
}  // namespace feature
