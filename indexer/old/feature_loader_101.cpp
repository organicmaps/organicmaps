#include "base/SRC_FIRST.hpp"

#include "indexer/old/feature_loader_101.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/scales.hpp"
#include "indexer/geometry_serialization.hpp"
#include "indexer/coding_params.hpp"

#include "coding/byte_stream.hpp"


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

namespace
{
  class TypeConvertor
  {
    vector<uint32_t> m_inc;

  public:
    TypeConvertor()
    {
      char const * arr[][3] = {
                                // first should be the new added type, after which all types should be incremented
                                { "shop", "convenience", "" },
                                { "shop", "hairdresser", "" },

                                { "highway", "minor", "oneway" },
                                { "highway", "minor", "tunnel" },
                                { "highway", "minor", "turning_circle" },

                                { "highway", "primary", "oneway" },
                                { "highway", "primary", "tunnel" },

                                { "highway", "primary_link", "oneway" },
                                { "highway", "primary_link", "tunnel" },

                                { "highway", "residential", "oneway" },
                                { "highway", "residential", "tunnel" },
                                { "highway", "residential", "turning_circle" },

                                { "highway", "road", "oneway" },
                                { "highway", "road", "tunnel" },
                                { "highway", "road", "turning_circle" },

                                { "highway", "secondary", "oneway" },
                                { "highway", "secondary", "tunnel" },

                                { "highway", "secondary_link", "oneway" },
                                { "highway", "secondary_link", "tunnel" },

                                { "highway", "service", "oneway" },
                                { "highway", "service", "parking_aisle" },
                                { "highway", "service", "tunnel" },

                                { "highway", "tertiary", "oneway" },
                                { "highway", "tertiary", "tunnel" },

                                { "highway", "tertiary_link", "oneway" },
                                { "highway", "tertiary_link", "tunnel" },

                                { "highway", "track", "oneway" },
                                { "highway", "track", "permissive" },
                                { "highway", "track", "private" },
                                { "highway", "track", "race" },
                                { "highway", "track", "racetrack" },
                                { "highway", "track", "tunnel" },

                                { "highway", "trunk", "oneway" },
                                { "highway", "trunk", "tunnel" },

                                { "highway", "trunk_link", "oneway" },
                                { "highway", "trunk_link", "tunnel" },

                                { "highway", "unclassified", "oneway" },
                                { "highway", "unclassified", "tunnel" },
                                { "highway", "unclassified", "turning_circle" },

                                { "highway", "unsurfaced", "oneway" },
                                { "highway", "unsurfaced", "permissive" },
                                { "highway", "unsurfaced", "private" },
                                { "highway", "unsurfaced", "tunnel" }
                              };

      Classificator const & c = classif();

      size_t const count = ARRAY_SIZE(arr);
      m_inc.reserve(count);

      for (size_t i = 0; i < count; ++i)
      {
        vector<string> v;
        v.push_back(arr[i][0]);
        v.push_back(arr[i][1]);
        if (strlen(arr[i][2]) > 0)
          v.push_back(arr[i][2]);

        m_inc.push_back(c.GetTypeByPath(v));
      }
    }

    uint32_t Convert(uint32_t t) const
    {
      // leave only 3 levels for compare
      ftype::TruncValue(t, 3);

      size_t const count = m_inc.size();
      for (size_t i = 0; i < count; ++i)
        if (m_inc[i] == t)
        {
          // return next type (advance by 1)
          ASSERT_LESS ( i+1, count, () );
          return m_inc[i+1];
        }
      return t;
    }
  };
}

void LoaderImpl::ParseTypes()
{
  ArrayByteSource source(DataPtr() + m_TypesOffset);

  static TypeConvertor typeC;

  size_t const count = m_pF->GetTypesCount();
  for (size_t i = 0; i < count; ++i)
    m_pF->m_types[i] = typeC.Convert(ReadVarUint<uint32_t>(source));

  m_CommonOffset = CalcOffset(source);
}

void LoaderImpl::ParseCommon()
{
  ArrayByteSource source(DataPtr() + m_CommonOffset);

  uint8_t const h = Header();

  if (h & HEADER_HAS_LAYER)
    m_pF->m_params.layer = ReadVarInt<int32_t>(source);

  if (h & HEADER_HAS_NAME)
  {
    string name;
    name.resize(ReadVarUint<uint32_t>(source) + 1);
    source.Read(&name[0], name.size());
    m_pF->m_params.name.AddString(StringUtf8Multilang::DEFAULT_CODE, name);
  }

  if (h & HEADER_HAS_POINT)
  {
    m_pF->m_center = Int64ToPoint(
          ReadVarInt<int64_t>(source) + GetDefCodingParams().GetBasePointInt64(), POINT_COORD_BITS);

    m_pF->m_limitRect.Add(m_pF->m_center);
  }

  m_Header2Offset = CalcOffset(source);
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
    int i = static_cast<int>(offsets.size()-1);
    while (i >= 0 && offsets[i] == s_InvalidOffset) --i;
    if (i >= 0)
      return i;
    else
      CHECK ( false, ("Feature should have any geometry ...") );
  }
  else
  {
    for (int i = 0; i < m_Info.GetScalesCount(); ++i)
      if (scale <= m_Info.GetScale(i))
      {
        if (offsets[i] != s_InvalidOffset)
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
  uint8_t ptsCount = 0, ptsMask = 0, trgCount = 0, trgMask = 0;

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

      char const * start = src.PtrC();

      src = ArrayByteSource(serial::LoadInnerPath(
                              start, ptsCount, GetDefCodingParams(), m_pF->m_points));

      m_pF->m_innerStats.m_points = static_cast<uint32_t>(src.PtrC() - start);
    }
    else
      ReadOffsets(src, ptsMask, m_ptsOffsets);
  }

  if (commonH & HEADER_IS_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = src.PtrC();

      FeatureType::points_t points;
      src = ArrayByteSource(serial::LoadInnerTriangles(
                              start, trgCount, GetDefCodingParams(), points));

      m_pF->m_innerStats.m_strips = static_cast<uint32_t>(src.PtrC() - start);

      for (uint8_t i = 2; i < trgCount; ++i)
      {
        m_pF->m_triangles.push_back(points[i-2]);
        m_pF->m_triangles.push_back(points[i-1]);
        m_pF->m_triangles.push_back(points[i]);
      }
    }
    else
      ReadOffsets(src, trgMask, m_trgOffsets);
  }

  m_pF->m_innerStats.m_size = static_cast<uint32_t>(src.PtrC() - DataPtr());
}

uint32_t LoaderImpl::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_IS_LINE)
  {
    if (m_pF->m_points.empty())
    {
      // outer geometry
      int const ind = GetScaleIndex(scale, m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetGeometryReader(ind));
        src.Skip(m_ptsOffsets[ind]);
        serial::LoadOuterPath(src, GetDefCodingParams(), m_pF->m_points);

        sz = static_cast<uint32_t>(src.Pos() - m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      size_t const count = m_pF->m_points.size();
      FeatureType::points_t points;
      points.reserve(count);

      int const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_Info.GetScalesCount(), () );

      points.push_back(m_pF->m_points.front());
      for (size_t i = 1; i < count-1; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (((m_ptsSimpMask >> (2*(i-1))) & 0x3) <= scaleIndex)
          points.push_back(m_pF->m_points[i]);
      }
      points.push_back(m_pF->m_points.back());

      m_pF->m_points.swap(points);
    }

    ::feature::CalcRect(m_pF->m_points, m_pF->m_limitRect);
  }

  return sz;
}

uint32_t LoaderImpl::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if (Header() & HEADER_IS_AREA)
  {
    if (m_pF->m_triangles.empty())
    {
      uint32_t const ind = GetScaleIndex(scale, m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(m_Info.GetTrianglesReader(ind));
        src.Skip(m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, GetDefCodingParams(), m_pF->m_triangles);

        sz = static_cast<uint32_t>(src.Pos() - m_trgOffsets[ind]);
      }
    }

    ::feature::CalcRect(m_pF->m_triangles, m_pF->m_limitRect);
  }

  return sz;
}

}
}
