#pragma once
#include "coding/varint.hpp"

#include "base/logging.hpp"

#include "std/limits.hpp"
#include "std/vector.hpp"

namespace feature
{
using TAltitude = int16_t;
using TAltitudes = vector<feature::TAltitude>;
using TAltitudeSectionVersion = uint16_t;
using TAltitudeSectionOffset = uint32_t;

TAltitude constexpr kInvalidAltitude = numeric_limits<TAltitude>::min();

struct AltitudeHeader
{
  AltitudeHeader()
  {
    Reset();
  }

  AltitudeHeader(TAltitudeSectionVersion v, TAltitude min, TAltitudeSectionOffset offset)
    : version(v), minAltitude(min), altitudeInfoOffset(offset)
  {
  }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    sink.Write(&version, sizeof(version));
    sink.Write(&minAltitude, sizeof(minAltitude));
    sink.Write(&altitudeInfoOffset, sizeof(altitudeInfoOffset));
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    src.Read(&version, sizeof(version));
    src.Read(&minAltitude, sizeof(minAltitude));
    src.Read(&altitudeInfoOffset, sizeof(altitudeInfoOffset));
  }

  static size_t GetHeaderSize()
  {
    return sizeof(version) + sizeof(minAltitude) + sizeof(altitudeInfoOffset);
  }

  void Reset()
  {
    version = 1;
    minAltitude = kInvalidAltitude;
    altitudeInfoOffset = 0;
  }

  TAltitudeSectionVersion version;
  TAltitude minAltitude;
  TAltitudeSectionOffset altitudeInfoOffset;
};

class Altitude
{
public:
  Altitude() = default;
  explicit Altitude(TAltitudes const & altitudes) : m_pointAlt(altitudes) {}

  template <class TSink>
  void Serialize(TAltitude minAltitude, TSink & sink) const
  {
    CHECK(!m_pointAlt.empty(), ());

    TAltitude prevPntAltitude = minAltitude;
    for (size_t i = 0; i < m_pointAlt.size(); ++i)
    {
      WriteVarInt(sink, static_cast<int32_t>(static_cast<int32_t>(m_pointAlt[i] - prevPntAltitude)));
      prevPntAltitude = m_pointAlt[i];
    }
  }

  template <class TSource>
  void Deserialize(TAltitude minAltitude, size_t pointCount, TSource & src)
  {
    m_pointAlt.clear();
    if (pointCount == 0)
    {
      ASSERT(false, ());
      return;
    }

    m_pointAlt.resize(pointCount);
    TAltitude prevPntAltitude = minAltitude;
    for (size_t i = 0; i < pointCount; ++i)
    {
      m_pointAlt[i] = static_cast<TAltitude>(ReadVarInt<int32_t>(src) + prevPntAltitude);
      prevPntAltitude = m_pointAlt[i];
    }
  }

  TAltitudes GetAltitudes() const
  {
    return m_pointAlt;
  }

private:
  /// \note |m_pointAlt| is a vector of feature point altitudes. There's two posibilities:
  /// * |m_pointAlt| is empty. It means no altitude information defines.
  /// * size of |m_pointAlt| is equal to feature point number. In that case every item of
  /// |m_pointAlt| defines altitude in meters for every feature point. If altitude is not defined
  ///  for some feature point corresponding vector items are equel to |kInvalidAltitude|
  TAltitudes m_pointAlt;
};
}  // namespace feature
