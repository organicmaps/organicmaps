#include "../base/SRC_FIRST.hpp"

#include "feature.hpp"
#include "feature_impl.hpp"
#include "feature_visibility.hpp"
#include "scales.hpp"
#include "geometry_coding.hpp"
#include "geometry_serialization.hpp"

#include "../defines.hpp" // just for file extensions

#include "../geometry/pointu_to_uint64.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/region2d.hpp"

#include "../coding/byte_stream.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilder1 implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FeatureBuilder1::IsGeometryClosed() const
{
  return (m_Geometry.size() > 2 && m_Geometry.front() == m_Geometry.back());
}

void FeatureBuilder1::SetCenter(m2::PointD const & p)
{
  m_Center = p;
  m_Params.SetGeomType(GEOM_POINT);
  m_LimitRect.Add(p);
}

void FeatureBuilder1::AddPoint(m2::PointD const & p)
{
  m_Geometry.push_back(p);
  m_LimitRect.Add(p);
}

void FeatureBuilder1::SetAreaAddHoles(list<points_t> const & holes)
{
  m_Params.SetGeomType(GEOM_AREA);
  m_Holes.clear();

  if (holes.empty()) return;

  m2::Region<m2::PointD> rgn(m_Geometry.begin(), m_Geometry.end());

  for (list<points_t>::const_iterator i = holes.begin(); i != holes.end(); ++i)
  {
    ASSERT ( !i->empty(), () );

    if (rgn.Contains(i->front()))
      m_Holes.push_back(*i);
  }
}

FeatureBase FeatureBuilder1::GetFeatureBase() const
{
  CHECK ( CheckValid(), () );

  FeatureBase f;
  f.SetHeader(m_Params.GetHeader());

  f.m_Params = m_Params;
  memcpy(f.m_Types, &m_Params.m_Types[0], sizeof(uint32_t) * m_Params.m_Types.size());
  f.m_LimitRect = m_LimitRect;

  f.m_bTypesParsed = f.m_bCommonParsed = true;

  return f;
}

namespace
{
  bool is_equal(double d1, double d2)
  {
    //return my::AlmostEqual(d1, d2, 100000000);
    return (fabs(d1 - d2) < MercatorBounds::GetCellID2PointAbsEpsilon());
  }

  bool is_equal(m2::PointD const & p1, m2::PointD const & p2)
  {
    return p1.EqualDxDy(p2, MercatorBounds::GetCellID2PointAbsEpsilon());
  }

  bool is_equal(m2::RectD const & r1, m2::RectD const & r2)
  {
    return (is_equal(r1.minX(), r2.minX()) &&
            is_equal(r1.minY(), r2.minY()) &&
            is_equal(r1.maxX(), r2.maxX()) &&
            is_equal(r1.maxY(), r2.maxY()));
  }

  bool is_equal(vector<m2::PointD> const & v1, vector<m2::PointD> const & v2)
  {
    if (v1.size() != v2.size())
      return false;

    for (size_t i = 0; i < v1.size(); ++i)
      if (!is_equal(v1[i], v2[i]))
        return false;

    return true;
  }
}

bool FeatureBuilder1::PreSerialize()
{
  if (!m_Params.IsValid()) return false;

  switch (m_Params.GetGeomType())
  {
  case GEOM_POINT:
    // If we don't have name and have house number, than replace them.
    if (m_Params.name.IsEmpty() && !m_Params.house.IsEmpty())
      m_Params.name.AddString(0, m_Params.house.Get());

    m_Params.ref = string();
    m_Params.house.Clear();
    break;

  case GEOM_LINE:
    // We need refs only for road numbers.
    if (!feature::IsHighway(m_Params.m_Types))
      m_Params.ref = string();

    m_Params.rank = 0;
    m_Params.house.Clear();
    break;

  case GEOM_AREA:
    m_Params.rank = 0;
    m_Params.ref = string();
    break;

  default:
    return false;
  }

  // Clear name for features with invisible texts.
  if (!m_Params.name.IsEmpty() && feature::MinDrawableScaleForText(GetFeatureBase()) == -1)
    m_Params.name.Clear();

  return true;
}

bool FeatureBuilder1::operator == (FeatureBuilder1 const & fb) const
{
  if (!(m_Params == fb.m_Params)) return false;

  if (m_Params.GetGeomType() == GEOM_POINT &&
      !is_equal(m_Center, fb.m_Center))
  {
    return false;
  }

  if (!is_equal(m_LimitRect, fb.m_LimitRect))
    return false;

  if (!is_equal(m_Geometry, fb.m_Geometry))
    return false;

  if (m_Holes.size() != fb.m_Holes.size())
    return false;

  list<points_t>::const_iterator i = m_Holes.begin();
  list<points_t>::const_iterator j = fb.m_Holes.begin();
  for (; i != m_Holes.end(); ++i, ++j)
    if (!is_equal(*i, *j))
      return false;

  return true;
}

bool FeatureBuilder1::CheckValid() const
{
  CHECK(m_Params.CheckValid(), ());

  EGeomType const type = m_Params.GetGeomType();

  CHECK(type != GEOM_LINE || m_Geometry.size() >= 2, ());

  CHECK(type != GEOM_AREA || m_Geometry.size() >= 3, ());

  CHECK(m_Holes.empty() || type == GEOM_AREA, ());

  for (list<points_t>::const_iterator i = m_Holes.begin(); i != m_Holes.end(); ++i)
    CHECK(i->size() >= 3, ());

  return true;
}

void FeatureBuilder1::SerializeBase(buffer_t & data, serial::CodingParams const & params) const
{
  PushBackByteSink<buffer_t> sink(data);

  m_Params.Write(sink);

  if (m_Params.GetGeomType() == GEOM_POINT)
    WriteVarUint(sink, EncodeDelta(PointD2PointU(m_Center.x, m_Center.y, params.GetCoordBits()),
                                   params.GetBasePoint()));
}

void FeatureBuilder1::Serialize(buffer_t & data) const
{
  CHECK ( CheckValid(), () );

  data.clear();

  SerializeBase(data, serial::CodingParams());

  PushBackByteSink<buffer_t> sink(data);

  EGeomType const type = m_Params.GetGeomType();

  if (type != GEOM_POINT)
    serial::SaveOuterPath(m_Geometry, serial::CodingParams(), sink);

  if (type == GEOM_AREA)
  {
    WriteVarUint(sink, uint32_t(m_Holes.size()));

    for (list<points_t>::const_iterator i = m_Holes.begin(); i != m_Holes.end(); ++i)
      serial::SaveOuterPath(*i, serial::CodingParams(), sink);
  }

  // check for correct serialization
#ifdef DEBUG
  buffer_t tmp(data);
  FeatureBuilder1 fb;
  fb.Deserialize(tmp);
  ASSERT ( fb == *this, () );
#endif
}

namespace
{
  template <class TCont>
  void CalcRect(TCont const & points, m2::RectD & rect)
  {
    for (size_t i = 0; i < points.size(); ++i)
      rect.Add(points[i]);
  }
}

void FeatureBuilder1::Deserialize(buffer_t & data)
{
  FeatureBase f;
  f.Deserialize(data, 0, serial::CodingParams());
  f.InitFeatureBuilder(*this);

  ArrayByteSource src(f.DataPtr() + f.m_Header2Offset);

  EGeomType const type = m_Params.GetGeomType();

  if (type != GEOM_POINT)
  {
    serial::LoadOuterPath(src, serial::CodingParams(), m_Geometry);
    CalcRect(m_Geometry, m_LimitRect);
  }

  if (type == GEOM_AREA)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    for (uint32_t i = 0; i < count; ++i)
    {
      m_Holes.push_back(points_t());
      serial::LoadOuterPath(src, serial::CodingParams(), m_Holes.back());
    }
  }

  CHECK ( CheckValid(), () );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilderGeomRef implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FeatureBuilder2::IsDrawableInRange(int lowS, int highS) const
{
  if (!m_Geometry.empty())
  {
    FeatureBase const fb = GetFeatureBase();

    while (lowS <= highS)
      if (feature::IsDrawableForIndex(fb, lowS++))
        return true;
  }

  return false;
}

bool FeatureBuilder2::PreSerialize(buffers_holder_t const & data)
{
  // make flags actual before header serialization
  if (data.m_ptsMask == 0 && data.m_innerPts.empty())
    m_Params.RemoveGeomType(GEOM_LINE);

  if (data.m_trgMask == 0 && data.m_innerTrg.empty())
    m_Params.RemoveGeomType(GEOM_AREA);

  // we don't need empty features without geometry
  return base_type::PreSerialize();
}

namespace
{
  template <class TSink> class BitSink
  {
    TSink & m_sink;
    uint8_t m_pos;
    uint8_t m_current;

  public:
    BitSink(TSink & sink) : m_sink(sink), m_pos(0), m_current(0) {}

    void Finish()
    {
      if (m_pos > 0)
      {
        WriteToSink(m_sink, m_current);
        m_pos = 0;
        m_current = 0;
      }
    }

    void Write(uint8_t value, uint8_t count)
    {
      ASSERT_LESS ( count, 9, () );
      ASSERT_EQUAL ( value >> count, 0, () );

      if (m_pos + count > 8)
        Finish();

      m_current |= (value << m_pos);
      m_pos += count;
    }
  };
}

void FeatureBuilder2::Serialize(buffers_holder_t & data, serial::CodingParams const & params)
{
  data.m_buffer.clear();

  // header data serialization
  SerializeBase(data.m_buffer, params);

  PushBackByteSink<buffer_t> sink(data.m_buffer);

  uint8_t const ptsCount = static_cast<uint8_t>(data.m_innerPts.size());
  uint8_t trgCount = static_cast<uint8_t>(data.m_innerTrg.size());
  if (trgCount > 0)
  {
    ASSERT_GREATER ( trgCount, 2, () );
    trgCount -= 2;
  }

  BitSink< PushBackByteSink<buffer_t> > bitSink(sink);

  uint8_t const h = m_Params.GetTypeMask();

  if (h & HEADER_GEOM_LINE)
  {
    bitSink.Write(ptsCount, 4);
    if (ptsCount == 0)
      bitSink.Write(data.m_ptsMask, 4);
  }

  if (h & HEADER_GEOM_AREA)
  {
    bitSink.Write(trgCount, 4);
    if (trgCount == 0)
      bitSink.Write(data.m_trgMask, 4);
  }

  bitSink.Finish();

  if (h & HEADER_GEOM_LINE)
  {
    if (ptsCount > 0)
    {
      if (ptsCount > 2)
      {
        uint32_t v = data.m_ptsSimpMask;
        int const count = (ptsCount - 2 + 3) / 4;
        for (int i = 0; i < count; ++i)
        {
          WriteToSink(sink, static_cast<uint8_t>(v));
          v >>= 8;
        }
      }

      serial::SaveInnerPath(data.m_innerPts, params, sink);
    }
    else
    {
      // offsets was pushed from high scale index to low
      reverse(data.m_ptsOffset.begin(), data.m_ptsOffset.end());
      serial::WriteVarUintArray(data.m_ptsOffset, sink);
    }
  }

  if (h & HEADER_GEOM_AREA)
  {
    if (trgCount > 0)
      serial::SaveInnerTriangles(data.m_innerTrg, params, sink);
    else
    {
      // offsets was pushed from high scale index to low
      reverse(data.m_trgOffset.begin(), data.m_trgOffset.end());
      serial::WriteVarUintArray(data.m_trgOffset, sink);
    }
  }
}

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

  ArrayByteSource source(DataPtr() + m_TypesOffset);
  for (size_t i = 0; i < GetTypesCount(); ++i)
    m_Types[i] = ReadVarUint<uint32_t>(source);

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

void FeatureBase::InitFeatureBuilder(FeatureBuilder1 & fb) const
{
  ParseAll();

  FeatureParams params(m_Params);
  params.AssignTypes(m_Types, m_Types + GetTypesCount());

  uint8_t const h = (Header() & HEADER_GEOTYPE_MASK);
  if (h & HEADER_GEOM_LINE) params.SetGeomType(GEOM_LINE);
  if (h & HEADER_GEOM_AREA) params.SetGeomType(GEOM_AREA);

  if (GetFeatureType() == GEOM_POINT)
  {
    fb.SetCenter(m_Center);
    params.SetGeomType(GEOM_POINT);
  }

  fb.SetParams(params);
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
        ReaderSource<FileReader> src(
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
        ReaderSource<FileReader> src(
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
  char const * m_priorities;
  string & m_result;
  int m_minPriority;

public:

  BestMatchedLangName(char const * priorities, string & result)
    : m_priorities(priorities), m_result(result), m_minPriority(256) {}
  bool operator() (char lang, string const & utf8s)
  {
    ASSERT(lang >= 0 && lang < MAX_SUPPORTED_LANGUAGES, ());
    int const priority = m_priorities[static_cast<uint8_t>(lang)];
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

string FeatureType::GetPreferredDrawableName(char const * priorities) const
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
