#pragma once
#include "coding/varint.hpp"

#include "base/assert.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/vector.hpp"

namespace feature
{
using TAltitude = int16_t;
using TAltitudes = vector<feature::TAltitude>;
using TAltitudeSectionOffset = uint32_t;

TAltitude constexpr kInvalidAltitude = numeric_limits<TAltitude>::min();

struct AltitudeHeader
{
  using TAltitudeSectionVersion = uint16_t;

  AltitudeHeader()
  {
    Reset();
  }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    sink.Write(&version, sizeof(version));
    sink.Write(&minAltitude, sizeof(minAltitude));
    sink.Write(&featureTableOffset, sizeof(featureTableOffset));
    sink.Write(&altitudeInfoOffset, sizeof(altitudeInfoOffset));
    sink.Write(&endOffset, sizeof(endOffset));
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    src.Read(&version, sizeof(version));
    src.Read(&minAltitude, sizeof(minAltitude));
    src.Read(&featureTableOffset, sizeof(featureTableOffset));
    src.Read(&altitudeInfoOffset, sizeof(altitudeInfoOffset));
    src.Read(&endOffset, sizeof(endOffset));
  }

  // Methods below return sizes of parts of altitude section in bytes.
  static size_t GetHeaderSize() { return sizeof(AltitudeHeader); }

  size_t GetAltitudeAvailabilitySize() const { return featureTableOffset - GetHeaderSize(); }
  size_t GetFeatureTableSize() const { return altitudeInfoOffset - featureTableOffset; }
  size_t GetAltitudeInfo() const { return endOffset - altitudeInfoOffset; }

  void Reset()
  {
    version = 1;
    minAltitude = kInvalidAltitude;
    featureTableOffset = 0;
    altitudeInfoOffset = 0;
    endOffset = 0;
  }

  TAltitudeSectionVersion version;
  TAltitude minAltitude;
  TAltitudeSectionOffset featureTableOffset;
  TAltitudeSectionOffset altitudeInfoOffset;
  TAltitudeSectionOffset endOffset;
};

class Altitude
{
public:
  Altitude() = default;
  explicit Altitude(TAltitudes const & altitudes) : m_altitudes(altitudes) {}

  template <class TSink>
  void Serialize(TAltitude minAltitude, TSink & sink) const
  {
    CHECK(!m_altitudes.empty(), ());

    WriteVarInt(sink, static_cast<int32_t>(m_altitudes[0] - static_cast<int32_t>(minAltitude)));
    for (size_t i = 1; i < m_altitudes.size(); ++i)
      WriteVarInt(sink, static_cast<int32_t>(m_altitudes[i]) - static_cast<int32_t>(m_altitudes[i - 1]));
  }

  template <class TSource>
  void Deserialize(TAltitude minAltitude, size_t pointCount, TSource & src)
  {
    m_altitudes.clear();
    if (pointCount == 0)
    {
      ASSERT(false, ());
      return;
    }

    m_altitudes.resize(pointCount);
    TAltitude prevPntAltitude = minAltitude;
    for (size_t i = 0; i < pointCount; ++i)
    {
      m_altitudes[i] = static_cast<TAltitude>(ReadVarInt<int32_t>(src) + prevPntAltitude);
      if (m_altitudes[i] < minAltitude)
      {
        ASSERT(false, ());
        m_altitudes.clear();
        return;
      }
      prevPntAltitude = m_altitudes[i];
    }
  }

  TAltitudes GetAltitudes() const
  {
    return m_altitudes;
  }

private:
  /// \note |m_altitudes| is a vector of feature point altitudes. There's two posibilities:
  /// * |m_altitudes| is empty. It means there is no altitude information for this feature.
  /// * size of |m_pointAlt| is equal to the number of this feature's points.
  /// In this case the i'th element of |m_pointAlt| corresponds to the altitude of the
  /// i'th point of the feature and is set to kInvalidAltitude when there is no information about the point.
  TAltitudes m_altitudes;
};
}  // namespace feature
