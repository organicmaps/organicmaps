#include "indexer/coding_params.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/pointd_to_pointu.hpp"

namespace serial
{
  CodingParams::CodingParams()
    : m_BasePointUint64(0), m_CoordBits(POINT_COORD_BITS)
  {
    m_BasePoint = Uint64ToPointUObsolete(m_BasePointUint64);
  }

  CodingParams::CodingParams(uint8_t coordBits, m2::PointD const & pt)
    : m_CoordBits(coordBits)
  {
    SetBasePoint(pt);
  }

  CodingParams::CodingParams(uint8_t coordBits, uint64_t basePointUint64)
    : m_BasePointUint64(basePointUint64), m_CoordBits(coordBits)
  {
    m_BasePoint = Uint64ToPointUObsolete(m_BasePointUint64);
  }

  void CodingParams::SetBasePoint(m2::PointD const & pt)
  {
    m_BasePoint = PointDToPointU(pt, m_CoordBits);
    m_BasePointUint64 = PointUToUint64Obsolete(m_BasePoint);
  }
}  // namespace serial
