#pragma once
#include "../geometry/point2d.hpp"

#include "../coding/varint.hpp"


namespace serial
{
  class CodingParams
  {
  public:
    // TODO: Factor out?
    CodingParams();
    CodingParams(uint8_t coordBits, m2::PointD const & pt);
    CodingParams(uint8_t coordBits, uint64_t basePointUint64);

    m2::PointU GetBasePointPrediction(uint64_t offset) const;

    // TODO: Factor out.
    m2::PointU GetBasePoint() const { return m_BasePoint; }
    // TODO: Factor out.
    int64_t GetBasePointInt64() const { return static_cast<int64_t>(m_BasePointUint64); }

    uint32_t GetCoordBits() const { return m_CoordBits; }

    template <typename WriterT> void Save(WriterT & writer) const
    {
      WriteVarUint(writer, GetCoordBits());
      WriteVarUint(writer, static_cast<uint64_t>(GetBasePointInt64()));
    }

    template <typename SourceT> void Load(SourceT & src)
    {
      uint32_t const coordBits = ReadVarUint<uint32_t>(src);
      ASSERT_LESS(coordBits, 32, ());
      uint64_t const basePointUint64 = ReadVarUint<uint64_t>(src);
      *this = CodingParams(coordBits, basePointUint64);
    }

  private:
    uint64_t m_BasePointUint64;
    // TODO: Factor out.
    m2::PointU m_BasePoint;
    uint8_t m_CoordBits;
  };
}
