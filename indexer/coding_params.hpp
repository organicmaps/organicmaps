#pragma once

#include "geometry/point2d.hpp"

#include "coding/varint.hpp"

namespace serial
{
  class CodingParams
  {
  public:
    /// @todo: Factor out?
    //@{
    CodingParams();
    CodingParams(uint8_t coordBits, m2::PointD const & pt);
    CodingParams(uint8_t coordBits, uint64_t basePointUint64);
    //@}

    /// @todo: Factor out.
    //@{
    inline m2::PointU GetBasePoint() const { return m_BasePoint; }
    inline uint64_t GetBasePointUint64() const { return m_BasePointUint64; }
    inline int64_t GetBasePointInt64() const
    {
      return static_cast<int64_t>(m_BasePointUint64);
    }

    void SetBasePoint(m2::PointD const & pt);
    //@}

    inline uint32_t GetCoordBits() const { return m_CoordBits; }

    template <typename WriterT> void Save(WriterT & writer) const
    {
      WriteVarUint(writer, GetCoordBits());
      WriteVarUint(writer, m_BasePointUint64);
    }

    template <typename SourceT> void Load(SourceT & src)
    {
      uint32_t const coordBits = ReadVarUint<uint32_t>(src);
      ASSERT_LESS(coordBits, 32, ());
      *this = CodingParams(coordBits, ReadVarUint<uint64_t>(src));
    }

  private:
    uint64_t m_BasePointUint64;
    m2::PointU m_BasePoint;
    uint8_t m_CoordBits;
  };
}  // namespace serial
