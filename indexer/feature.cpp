#include "../base/SRC_FIRST.hpp"

#include "feature.hpp"
#include "feature_impl.hpp"
#include "feature_visibility.hpp"
#include "scales.hpp"
#include "geometry_coding.hpp"
#include "geometry_serialization.hpp"

#include "../defines.hpp" // just for file extensions

#include "../coding/byte_stream.hpp"

#include "../geometry/pointu_to_uint64.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/region2d.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(buffer_t & data, uint32_t offset, serial::CodingParams const & params)
{
  m_Offset = offset;
  m_Data.swap(data);

  m_CodingParams = params;

  m_CommonOffset = m_Header2Offset = 0;
  m_bTypesParsed = m_bCommonParsed = false;

  m_Params = FeatureParamsBase();
  m_LimitRect = m2::RectD::GetEmptyRect();
}

uint32_t FeatureBase::CalcOffset(ArrayByteSource const & source) const
{
  return static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr());
}

void FeatureBase::SetHeader(uint8_t h)
{
  ASSERT ( m_Offset == 0, (m_Offset) );
  m_Data.resize(1);
  m_Data[0] = h;
}

feature::EGeomType FeatureBase::GetFeatureType() const
{
  uint8_t const h = (Header() & HEADER_GEOTYPE_MASK);

  if (h & HEADER_GEOM_AREA)
    return GEOM_AREA;
  else if (h & HEADER_GEOM_LINE)
    return GEOM_LINE;
  else
  {
    ASSERT ( h == HEADER_GEOM_POINT, (h) );
    return GEOM_POINT;
  }
}

void FeatureBase::ParseTypes() const
{
  ASSERT(!m_bTypesParsed, ());

  Classificator & c = classif();

  ArrayByteSource source(DataPtr() + m_TypesOffset);
  for (size_t i = 0; i < GetTypesCount(); ++i)
  {
    m_Types[i] = c.GetTypeForIndex(ReadVarUint<uint32_t>(source));
    //m_Types[i] = ReadVarUint<uint32_t>(source);
  }

  m_bTypesParsed = true;
  m_CommonOffset = CalcOffset(source);
}

void FeatureBase::ParseCommon() const
{
  ASSERT(!m_bCommonParsed, ());
  if (!m_bTypesParsed)
    ParseTypes();

  ArrayByteSource source(DataPtr() + m_CommonOffset);

  uint8_t const h = Header();

  EGeomType const type = GetFeatureType();

  m_Params.Read(source, h, type);

  if (type == GEOM_POINT)
  {
    CoordPointT center = PointU2PointD(DecodeDelta(ReadVarUint<uint64_t>(source),
                                                   m_CodingParams.GetBasePoint()),
                                       m_CodingParams.GetCoordBits());
    m_Center = m2::PointD(center.first, center.second);
    m_LimitRect.Add(m_Center);
  }

  m_bCommonParsed = true;
  m_Header2Offset = CalcOffset(source);
}

void FeatureBase::ParseAll() const
{
  if (!m_bCommonParsed)
    ParseCommon();
}

string FeatureBase::DebugString() const
{
  ASSERT(m_bCommonParsed, ());

  string res("FEATURE: ");

  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += "Type:" + debug_print(m_Types[i]) + " ";

  res += m_Params.DebugString();

  if (GetFeatureType() == GEOM_POINT)
    res += "Center:" + debug_print(m_Center) + " ";

  return res;
}

FeatureParams FeatureBase::GetFeatureParams() const
{
  FeatureParams params(m_Params);
  params.AssignTypes(m_Types, m_Types + GetTypesCount());

  uint8_t const h = (Header() & HEADER_GEOTYPE_MASK);
  if (h & HEADER_GEOM_LINE) params.SetGeomType(GEOM_LINE);
  if (h & HEADER_GEOM_AREA) params.SetGeomType(GEOM_AREA);

  return params;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureType implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureType::FeatureType(read_source_t & src)
{
  Deserialize(src);
}

void FeatureType::Deserialize(read_source_t & src)
{
  m_cont = &src.m_cont;
  m_header = &src.m_header;

  m_Points.clear();
  m_Triangles.clear();

  m_bHeader2Parsed = m_bPointsParsed = m_bTrianglesParsed = false;
  m_ptsSimpMask = 0;

  m_InnerStats.MakeZero();

  base_type::Deserialize(src.m_data, src.m_offset, m_header->GetCodingParams());
}

namespace
{
    uint32_t const kInvalidOffset = uint32_t(-1);
}

int FeatureType::GetScaleIndex(int scale) const
{
  int const count = m_header->GetScalesCount();

  switch (scale)
  {
  case -2: return 0;
  case -1: return count-1;
  default:
    for (int i = 0; i < count; ++i)
      if (scale <= m_header->GetScale(i))
        return i;
    return -1;
  }
}

int FeatureType::GetScaleIndex(int scale, offsets_t const & offsets) const
{
  int ind = -1;
  int const count = static_cast<int>(offsets.size());

  switch (scale)
  {
  case -2:
    // Choose the worst existing geometry.
    ind = count-1;
    while (ind >= 0 && offsets[ind] == kInvalidOffset) --ind;
    break;

  case -1:
    // Choose the best geometry for the last visible scale.
    ind = 0;
    while (ind < count && offsets[ind] == kInvalidOffset) ++ind;
    break;

  default:
    for (size_t i = 0; i < m_header->GetScalesCount(); ++i)
    {
      if (scale <= m_header->GetScale(i))
        return (offsets[i] != kInvalidOffset ? i : -1);
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

namespace
{
  template <class TCont>
  void Points2String(string & s, TCont const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
      s += debug_print(points[i]) + " ";
  }
}

string FeatureType::DebugString(int scale) const
{
  ParseAll(scale);

  string s = base_type::DebugString();

  s += "Points:";
  Points2String(s, m_Points);

  s += "Triangles:";
  Points2String(s, m_Triangles);

  return s;
}

bool FeatureType::IsEmptyGeometry(int scale) const
{
  ParseAll(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_Triangles.empty();
  case GEOM_LINE: return m_Points.empty();
  default: return false;
  }
}

m2::RectD FeatureType::GetLimitRect(int scale) const
{
  ParseAll(scale);

  if (m_Triangles.empty() && m_Points.empty() && (GetFeatureType() != GEOM_POINT))
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume that rect has zero dimensions.
    m_LimitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_LimitRect;
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

void FeatureType::ParseHeader2() const
{
  ASSERT(!m_bHeader2Parsed, ());
  if (!m_bCommonParsed)
    ParseCommon();

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

      src = ArrayByteSource(serial::LoadInnerPath(src.Ptr(), ptsCount, m_CodingParams, m_Points));

      m_InnerStats.m_Points = static_cast<char const *>(src.Ptr()) - start;
    }
    else
      ReadOffsets(src, ptsMask, m_ptsOffsets);
  }

  if (h & HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
    {
      trgCount += 2;

      char const * start = static_cast<char const *>(src.Ptr());

      points_t points;
      src = ArrayByteSource(serial::LoadInnerTriangles(src.Ptr(), trgCount, m_CodingParams, points));

      m_InnerStats.m_Strips = static_cast<char const *>(src.Ptr()) - start;

      for (uint8_t i = 2; i < trgCount; ++i)
      {
        m_Triangles.push_back(points[i-2]);
        m_Triangles.push_back(points[i-1]);
        m_Triangles.push_back(points[i]);
      }
    }
    else
      ReadOffsets(src, trgMask, m_trgOffsets);
  }

  m_bHeader2Parsed = true;
  m_InnerStats.m_Size = static_cast<char const *>(src.Ptr()) - DataPtr();
}

uint32_t FeatureType::ParseGeometry(int scale) const
{
  ASSERT(!m_bPointsParsed, ());
  if (!m_bHeader2Parsed)
    ParseHeader2();

  uint32_t sz = 0;
  if (Header() & HEADER_GEOM_LINE)
  {
    if (m_Points.empty())
    {
      // outer geometry
      int const ind = GetScaleIndex(scale, m_ptsOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(
              m_cont->GetReader(feature::GetTagForIndex(GEOMETRY_FILE_TAG, ind)));
        src.Skip(m_ptsOffsets[ind]);
        serial::LoadOuterPath(src, m_CodingParams, m_Points);

        sz = static_cast<uint32_t>(src.Pos() - m_ptsOffsets[ind]);
      }
    }
    else
    {
      // filter inner geometry

      size_t const count = m_Points.size();
      points_t points;
      points.reserve(count);

      uint32_t const scaleIndex = GetScaleIndex(scale);
      ASSERT_LESS ( scaleIndex, m_header->GetScalesCount(), () );

      points.push_back(m_Points.front());
      for (size_t i = 1; i < count-1; ++i)
      {
        // check for point visibility in needed scaleIndex
        if (((m_ptsSimpMask >> (2*(i-1))) & 0x3) <= scaleIndex)
          points.push_back(m_Points[i]);
      }
      points.push_back(m_Points.back());

      m_Points.swap(points);
    }

    CalcRect(m_Points, m_LimitRect);
  }

  m_bPointsParsed = true;
  return sz;
}

uint32_t FeatureType::ParseTriangles(int scale) const
{
  ASSERT(!m_bTrianglesParsed, ());
  if (!m_bHeader2Parsed)
    ParseHeader2();

  uint32_t sz = 0;
  if (Header() & HEADER_GEOM_AREA)
  {
    if (m_Triangles.empty())
    {
      uint32_t const ind = GetScaleIndex(scale, m_trgOffsets);
      if (ind != -1)
      {
        ReaderSource<FilesContainerR::ReaderT> src(
              m_cont->GetReader(feature::GetTagForIndex(TRIANGLE_FILE_TAG, ind)));
        src.Skip(m_trgOffsets[ind]);
        serial::LoadOuterTriangles(src, m_CodingParams, m_Triangles);

        sz = static_cast<uint32_t>(src.Pos() - m_trgOffsets[ind]);
      }
    }

    CalcRect(m_Triangles, m_LimitRect);
  }

  m_bTrianglesParsed = true;
  return sz;
}

void FeatureType::ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets) const
{
  ASSERT_GREATER ( mask, 0, () );

  size_t index = 0;
  while (mask > 0)
  {
    ASSERT_LESS ( index, m_header->GetScalesCount(), () );
    offsets[index++] = (mask & 0x01) ? ReadVarUint<uint32_t>(src) : kInvalidOffset;
    mask = mask >> 1;
  }

  while (index < offsets.size())
    offsets[index++] = kInvalidOffset;
}

void FeatureType::ParseAll(int scale) const
{
  if (!m_bPointsParsed)
    ParseGeometry(scale);

  if (!m_bTrianglesParsed)
    ParseTriangles(scale);
}

FeatureType::geom_stat_t FeatureType::GetGeometrySize(int scale) const
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_Points.empty())
    sz = m_InnerStats.m_Points;

  return geom_stat_t(sz, m_Points.size());
}

FeatureType::geom_stat_t FeatureType::GetTrianglesSize(int scale) const
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_Triangles.empty())
    sz = m_InnerStats.m_Strips;

  return geom_stat_t(sz, m_Triangles.size());
}

class BestMatchedLangName
{
  int8_t const * m_priorities;
  string & m_result;
  int m_minPriority;

public:

  BestMatchedLangName(int8_t const * priorities, string & result)
    : m_priorities(priorities), m_result(result), m_minPriority(256) {}
  bool operator() (int8_t lang, string const & utf8s)
  {
    ASSERT(lang >= 0 && lang < MAX_SUPPORTED_LANGUAGES, ());
    int8_t const priority = m_priorities[lang];
    if (priority == 0)
    {
      m_result = utf8s;
      return false; // stop foreach
    }
    if (priority < m_minPriority)
    {
      m_minPriority = priority;
      m_result = utf8s;
    }
    return true;
  }
};

string FeatureType::GetPreferredDrawableName(int8_t const * priorities) const
{
  if (!m_bCommonParsed)
    ParseCommon();

  string res;
  if (priorities)
  {
    BestMatchedLangName matcher(priorities, res);
    ForEachNameRef(matcher);
  }
  else
    m_Params.name.GetString(0, res);

  if (res.empty() && GetFeatureType() == GEOM_AREA)
    res = m_Params.house.Get();

  return res;
}

uint32_t FeatureType::GetPopulation() const
{
  if (!m_bCommonParsed)
    ParseCommon();

  if (m_Params.rank == 0)
    return 1;

  return static_cast<uint32_t>(min(double(uint32_t(-1)), pow(1.1, m_Params.rank)));
}

double FeatureType::GetPopulationDrawRank() const
{
  uint32_t const n = GetPopulation();
  if (n == 1) return 0.0;

  // Do not return rank for countries.
  if (feature::IsCountry(m_Types, m_Types + GetTypesCount()))
    return 0.0;
  else
    return min(3.0E6, static_cast<double>(n)) / 3.0E6;
}

uint8_t FeatureType::GetSearchRank() const
{
  if (!m_bCommonParsed)
    ParseCommon();
  return m_Params.rank;
}
