#include "../base/SRC_FIRST.hpp"

#include "feature_loader.hpp"
#include "feature.hpp"
#include "scales.hpp"
#include "geometry_serialization.hpp"
#include "classificator.hpp"

#include "../geometry/pointu_to_uint64.hpp"

#include "../coding/byte_stream.hpp"

#include "../base/start_mem_debug.hpp"


namespace feature
{

uint8_t LoaderCurrent::GetHeader()
{
  return Header();
}

void LoaderCurrent::ParseTypes()
{
  Classificator & c = classif();

  ArrayByteSource source(DataPtr() + m_TypesOffset);

  size_t const count = m_pF->GetTypesCount();
  for (size_t i = 0; i < count; ++i)
    m_pF->m_Types[i] = c.GetTypeForIndex(ReadVarUint<uint32_t>(source));

  m_CommonOffset = CalcOffset(source);
}

void LoaderCurrent::ParseCommon()
{
  ArrayByteSource source(DataPtr() + m_CommonOffset);

  uint8_t const h = Header();

  EGeomType const type = m_pF->GetFeatureType();

  m_pF->m_Params.Read(source, h, type);

  if (type == GEOM_POINT)
  {
    m_pF->m_Center = serial::LoadPoint(source, GetDefCodingParams());
    m_pF->m_LimitRect.Add(m_pF->m_Center);
  }

  m_Header2Offset = CalcOffset(source);
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

void LoaderCurrent::ParseHeader2()
{
  uint8_t ptsCount, ptsMask, trgCount, trgMask;

  BitSource bitSource(DataPtr() + m_Header2Offset);

  uint8_t const h = (Header() & HEADER_GEOTYPE_MASK);

  if (h & HEADER_GEOM_LINE)
  {
    ptsCount = bitSource.Read(4);
    if (ptsCount == 0)
      ptsMask = bitSource.Read(4);
    else
    {
      ASSERT_GREATER ( ptsCount, 1, () );
    }
  }

  if (h & HEADER_GEOM_AREA)
  {
    trgCount = bitSource.Read(4);
    if (trgCount == 0)
      trgMask = bitSource.Read(4);
  }

  ArrayByteSource src(bitSource.RoundPtr());

  serial::CodingParams const & cp = GetDefCodingParams();

  if (h & HEADER_GEOM_LINE)
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

      src = ArrayByteSource(serial::LoadInnerPath(src.Ptr(), ptsCount, cp, m_pF->m_Points));

      m_pF->m_InnerStats.m_Points = static_cast<char const *>(src.Ptr()) - start;
    }
    else
    {
      m_pF->m_Points.push_back(serial::LoadPoint(src, cp));

      ReadOffsets(src, ptsMask, m_ptsOffsets);
    }
  }

  if (h & HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.Ptr());

      FeatureType::points_t points;
      src = ArrayByteSource(serial::LoadInnerTriangles(src.Ptr(), trgCount, cp, points));

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

uint32_t LoaderCurrent::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_GEOM_LINE)
  {
    size_t const count = m_pF->m_Points.size();
    if (count < 2)
    {
      ASSERT_EQUAL ( count, 1, () );

      // outer geometry
      int const ind = GetScaleIndex(scale, m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetGeometryReader(ind));
        src.Skip(m_ptsOffsets[ind]);

        serial::CodingParams cp = GetCodingParams(ind);
        cp.SetBasePoint(m_pF->m_Points[0]);
        serial::LoadOuterPath(src, cp, m_pF->m_Points);

        sz = static_cast<uint32_t>(src.Pos() - m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      FeatureType::points_t points;
      points.reserve(count);

      int const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_Info.GetScalesCount(), () );

      points.push_back(m_pF->m_Points.front());
      for (int i = 1; i < count-1; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (((m_ptsSimpMask >> (2*(i-1))) & 0x3) <= scaleIndex)
          points.push_back(m_pF->m_Points[i]);
      }
      points.push_back(m_pF->m_Points.back());

      m_pF->m_Points.swap(points);
    }

    CalcRect(m_pF->m_Points, m_pF->m_LimitRect);
  }

  return sz;
}

uint32_t LoaderCurrent::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_GEOM_AREA)
  {
    if (m_pF->m_Triangles.empty())
    {
      uint32_t const ind = GetScaleIndex(scale, m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetTrianglesReader(ind));
        src.Skip(m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, GetCodingParams(ind), m_pF->m_Triangles);

        sz = static_cast<uint32_t>(src.Pos() - m_trgOffsets[ind]);
      }
    }

    CalcRect(m_pF->m_Triangles, m_pF->m_LimitRect);
  }

  return sz;
}

namespace
{
    uint32_t const kInvalidOffset = uint32_t(-1);
}

void LoaderCurrent::ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets) const
{
  ASSERT_GREATER ( mask, 0, () );

  int index = 0;
  while (mask > 0)
  {
    ASSERT_LESS ( index, m_Info.GetScalesCount(), () );
    offsets[index++] = (mask & 0x01) ? ReadVarUint<uint32_t>(src) : kInvalidOffset;
    mask = mask >> 1;
  }

  while (index < offsets.size())
    offsets[index++] = kInvalidOffset;
}

int LoaderCurrent::GetScaleIndex(int scale) const
{
  int const count = m_Info.GetScalesCount();

  switch (scale)
  {
  case -2: return 0;
  case -1: return count-1;
  default:
    for (int i = 0; i < count; ++i)
      if (scale <= m_Info.GetScale(i))
        return i;
    return -1;
  }
}

int LoaderCurrent::GetScaleIndex(int scale, offsets_t const & offsets) const
{
  int ind = -1;
  int const count = static_cast<int>(offsets.size());

  switch (scale)
  {
  case -1:
    // Choose the best existing geometry for the last visible scale.
    ind = count-1;
    while (ind >= 0 && offsets[ind] == kInvalidOffset) --ind;
    break;

  case -2:
    // Choose the worst existing geometry for the first visible scale.
    ind = 0;
    while (ind < count && offsets[ind] == kInvalidOffset) ++ind;
    break;

  default:
    {
      int const count = m_Info.GetScalesCount();
      for (int i = 0; i < count; ++i)
      {
        if (scale <= m_Info.GetScale(i))
          return (offsets[i] != kInvalidOffset ? i : -1);
      }
    }
  }

  if (ind >= 0 && ind < count)
    return ind;
  else
  {
    CHECK ( false, ("Feature should have any geometry ...") );
    return -1;
  }
}

}
