#include "indexer/feature_altitude.hpp"

#include "base/bits.hpp"

namespace feature
{
void Altitudes::PrepareSerializationData(TAltitude minAltitude, vector<uint32_t> & deviations) const
{
  CHECK(!m_altitudes.empty(), ());

  deviations.clear();
  uint32_t const firstPntDeviation =
      bits::ZigZagEncode(static_cast<int32_t>(m_altitudes[0]) -
      static_cast<int32_t>(minAltitude)) + 1 /* making it more than zero */;
  CHECK_LESS(0, firstPntDeviation, ());
  deviations.push_back(firstPntDeviation);
  for (size_t i = 1; i < m_altitudes.size(); ++i)
  {
    uint32_t const nextPntDeviation = bits::ZigZagEncode(static_cast<int32_t>(m_altitudes[i]) -
        static_cast<int32_t>(m_altitudes[i - 1])) + 1 /* making it more than zero */;
    CHECK_LESS(0, nextPntDeviation, ());
    deviations.push_back(nextPntDeviation);
  }
}

bool Altitudes::FillAltitudesByDeserializedDate(TAltitude minAltitude, vector<uint32_t> const & deviations)
{
  m_altitudes.clear();
  if (deviations.size() == 0)
  {
    ASSERT(false, ());
    return false;
  }

  m_altitudes.resize(deviations.size());
  TAltitude prevAltitude = minAltitude;
  for (size_t i = 0; i < deviations.size(); ++i)
  {
    if (deviations[i] == 0)
    {
      ASSERT(false, (i));
      m_altitudes.clear();
      return false;
    }

    m_altitudes[i] = static_cast<TAltitude>(bits::ZigZagDecode(deviations[i] -
                                                               1 /* Recovering value */) + prevAltitude);
    if (m_altitudes[i] < minAltitude)
    {
      ASSERT(false, (i));
      m_altitudes.clear();
      return false;
    }
    prevAltitude = m_altitudes[i];
  }
  return true;
}
}  // namespace feature
