#include "../../base/SRC_FIRST.hpp"

#include "feature_loader_101.hpp"

#include "../feature.hpp"
#include "../scales.hpp"
#include "../geometry_serialization.hpp"
#include "../coding_params.hpp"

#include "../../coding/byte_stream.hpp"

#include "../../base/start_mem_debug.hpp"


namespace old_101 { namespace feature {


uint8_t LoaderImpl::GetHeader()
{
  uint8_t const h = Header();

  uint8_t header = static_cast<uint8_t>((h & 7) - 1);

  if (h & HEADER_HAS_NAME)
    header |= ::feature::HEADER_HAS_NAME;

  if (h & HEADER_HAS_LAYER)
    header |= ::feature::HEADER_HAS_LAYER;

  if (h & HEADER_IS_LINE)
    header |= ::feature::HEADER_GEOM_LINE;

  if (h & HEADER_IS_AREA)
    header |= ::feature::HEADER_GEOM_AREA;

  return header;
}

void LoaderImpl::ParseTypes()
{
  ArrayByteSource source(DataPtr() + m_TypesOffset);

  size_t const count = m_pF->GetTypesCount();
  for (size_t i = 0; i < count; ++i)
    m_pF->m_Types[i] = ReadVarUint<uint32_t>(source);

  m_CommonOffset = CalcOffset(source);
}

void LoaderImpl::ParseCommon()
{
  ArrayByteSource source(DataPtr() + m_CommonOffset);

  uint8_t const h = Header();

  if (h & HEADER_HAS_LAYER)
    m_pF->m_Params.layer = ReadVarInt<int32_t>(source);

  if (h & HEADER_HAS_NAME)
  {
    string name;
    name.resize(ReadVarUint<uint32_t>(source) + 1);
    source.Read(&name[0], name.size());
    m_pF->m_Params.name.AddString(0, name);
  }

  if (h & HEADER_HAS_POINT)
  {
    CoordPointT const center = Int64ToPoint(
          ReadVarInt<int64_t>(source) + m_Info.GetCodingParams().GetBasePointInt64(), POINT_COORD_BITS);

    m_pF->m_Center = m2::PointD(center.first, center.second);
    m_pF->m_LimitRect.Add(m_pF->m_Center);
  }

  m_Header2Offset = CalcOffset(source);
}

namespace
{
    uint32_t const kInvalidOffset = uint32_t(-1);
}

int LoaderImpl::GetScaleIndex(int scale) const
{
  int const count = m_Info.GetScalesCount();
  if (scale == -1) return count-1;

  for (int i = 0; i < count; ++i)
    if (scale <= m_Info.GetScale(i))
      return i;
  return -1;
}

int LoaderImpl::GetScaleIndex(int scale, offsets_t const & offsets) const
{
  if (scale == -1)
  {
    // Choose the best geometry for the last visible scale.
    int i = offsets.size()-1;
    while (i >= 0 && offsets[i] == kInvalidOffset) --i;
    if (i >= 0)
      return i;
    else
      CHECK ( false, ("Feature should have any geometry ...") );
  }
  else
  {
    for (size_t i = 0; i < m_Info.GetScalesCount(); ++i)
      if (scale <= m_Info.GetScale(i))
      {
        if (offsets[i] != kInvalidOffset)
          return i;
        else
          break;
      }
  }

  return -1;
}

namespace
{
  class BitSource
  {
    char const * m_ptr;
    uint8_t m_pos;

  public:
    BitSource(char const * p) : m_ptr(p), m_pos(0) {}

    uint8_t Read(uint8_t count)
    {
      ASSERT_LESS ( count, 9, () );

      uint8_t v = *m_ptr;
      v >>= m_pos;
      v &= ((1 << count) - 1);

      m_pos += count;
      if (m_pos >= 8)
      {
        ASSERT_EQUAL ( m_pos, 8, () );
        ++m_ptr;
        m_pos = 0;
      }

      return v;
    }

    char const * RoundPtr()
    {
      if (m_pos > 0)
      {
        ++m_ptr;
        m_pos = 0;
      }
      return m_ptr;
    }
  };

  template <class TSource> uint8_t ReadByte(TSource & src)
  {
    return ReadPrimitiveFromSource<uint8_t>(src);
  }
}

void LoaderImpl::ParseHeader2()
{
  uint8_t ptsCount, ptsMask, trgCount, trgMask;

  uint8_t const commonH = Header();
  BitSource bitSource(DataPtr() + m_Header2Offset);

  if (commonH & HEADER_IS_LINE)
  {
    ptsCount = bitSource.Read(4);
    if (ptsCount == 0)
      ptsMask = bitSource.Read(4);
    else
    {
      ASSERT_GREATER ( ptsCount, 1, () );
    }
  }

  if (commonH & HEADER_IS_AREA)
  {
    trgCount = bitSource.Read(4);
    if (trgCount == 0)
      trgMask = bitSource.Read(4);
  }

  ArrayByteSource src(bitSource.RoundPtr());

  if (commonH & HEADER_IS_LINE)
  {
    if (ptsCount > 0)
    {
      int const count = (ptsCount - 2 + 3) / 4;
      ASSERT_LESS ( count, 4, () );

      for (int i = 0; i < count; ++i)
      {
        uint32_t mask = ReadByte(src);
        m_ptsSimpMask += (mask << (i << 3));
      }

      char const * start = static_cast<char const *>(src.Ptr());

      src = ArrayByteSource(serial::LoadInnerPath(
                              src.Ptr(), ptsCount, m_Info.GetCodingParams(), m_pF->m_Points));

      m_pF->m_InnerStats.m_Points = static_cast<char const *>(src.Ptr()) - start;
    }
    else
      ReadOffsets(src, ptsMask, m_ptsOffsets);
  }

  if (commonH & HEADER_IS_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.Ptr());

      FeatureType::points_t points;
      src = ArrayByteSource(serial::LoadInnerTriangles(
                              src.Ptr(), trgCount, m_Info.GetCodingParams(), points));

      m_pF->m_InnerStats.m_Strips = static_cast<char const *>(src.Ptr()) - start;

      for (uint8_t i = 2; i < trgCount; ++i)
      {
        m_pF->m_Triangles.push_back(points[i-2]);
        m_pF->m_Triangles.push_back(points[i-1]);
        m_pF->m_Triangles.push_back(points[i]);
      }
    }
    else
      ReadOffsets(src, trgMask, m_trgOffsets);
  }

  m_pF->m_InnerStats.m_Size = static_cast<char const *>(src.Ptr()) - DataPtr();
}

uint32_t LoaderImpl::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_IS_LINE)
  {
    if (m_pF->m_Points.empty())
    {
      // outer geometry
      int const ind = GetScaleIndex(scale, m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetGeometryReader(ind));
        src.Skip(m_ptsOffsets[ind]);
        serial::LoadOuterPath(src, m_Info.GetCodingParams(), m_pF->m_Points);

        sz = static_cast<uint32_t>(src.Pos() - m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      size_t const count = m_pF->m_Points.size();
      FeatureType::points_t points;
      points.reserve(count);

      int const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_Info.GetScalesCount(), () );

      points.push_back(m_pF->m_Points.front());
      for (size_t i = 1; i < count-1; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (((m_ptsSimpMask >> (2*(i-1))) & 0x3) <= scaleIndex)
          points.push_back(m_pF->m_Points[i]);
      }
      points.push_back(m_pF->m_Points.back());

      m_pF->m_Points.swap(points);
    }

    ::feature::CalcRect(m_pF->m_Points, m_pF->m_LimitRect);
  }

  return sz;
}

uint32_t LoaderImpl::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_IS_AREA)
  {
    if (m_pF->m_Triangles.empty())
    {
      uint32_t const ind = GetScaleIndex(scale, m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetTrianglesReader(ind));
        src.Skip(m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, m_Info.GetCodingParams(), m_pF->m_Triangles);

        sz = static_cast<uint32_t>(src.Pos() - m_trgOffsets[ind]);
      }
    }

    ::feature::CalcRect(m_pF->m_Triangles, m_pF->m_LimitRect);
  }

  return sz;
}

void LoaderImpl::ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets) const
{
  ASSERT_GREATER ( mask, 0, () );

  int index = 0;
  while (mask > 0)
  {
    ASSERT_LESS ( index, m_Info.GetScalesCount(), () );
    offsets[index++] = (mask & 0x01) ? ReadVarUint<uint32_t>(src) : kInvalidOffset;
    mask = mask >> 1;
  }
}

}
}
