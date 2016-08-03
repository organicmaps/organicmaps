#include "indexer/feature_altitude.hpp"

#include "base/bits.hpp"

namespace feature
{
void Altitudes::PrepareSerializationDate(TAltitude minAltitude, vector<uint32_t> & deltas) const
{
  CHECK(!m_altitudes.empty(), ());
  CHECK_LESS_OR_EQUAL(minAltitude, m_altitudes[0], ());

  deltas.resize(m_altitudes.size());
  uint32_t const firstPntDeviation =
      bits::ZigZagEncode(static_cast<int32_t>(m_altitudes[0]) -
      static_cast<int32_t>(minAltitude));

  deltas[0] = firstPntDeviation;
  for (size_t i = 1; i < m_altitudes.size(); ++i)
  {
    CHECK_LESS_OR_EQUAL(minAltitude, m_altitudes[i], (i));
    deltas[i] = bits::ZigZagEncode(static_cast<int32_t>(m_altitudes[i]) -
        static_cast<int32_t>(m_altitudes[i - 1]));
  }
}

bool Altitudes::FillAltitudesByDeserializedDate(TAltitude minAltitude, vector<uint32_t> const & deltas)
{
  m_altitudes.resize(deltas.size());
  if (deltas.size() == 0)
  {
    ASSERT(false, ("A vector of delta altitudes readed from mwm are empty."));
    m_altitudes.clear();
    return false;
  }

  TAltitude prevAltitude = minAltitude;
  for (size_t i = 0; i < deltas.size(); ++i)
  {
    m_altitudes[i] = static_cast<TAltitude>(bits::ZigZagDecode(deltas[i]) + prevAltitude);
    if (m_altitudes[i] < minAltitude)
    {
      ASSERT(false, ("A point altitude readed from file is less then min mwm altitude. Point number in its feature is", i));
      m_altitudes.clear();
      return false;
    }
    prevAltitude = m_altitudes[i];
  }
  return true;
}
}  // namespace feature
