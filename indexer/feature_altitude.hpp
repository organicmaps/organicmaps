#pragma once

#include "coding/varint.hpp"

#include "std/limits.hpp"
#include "std/vector.hpp"

namespace feature
{
using TAltitude = int16_t;
using TAltitudes = vector<feature::TAltitude>;
TAltitude constexpr kInvalidAltitude = numeric_limits<TAltitude>::min();

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
