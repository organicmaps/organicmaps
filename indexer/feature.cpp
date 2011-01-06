#include "../base/SRC_FIRST.hpp"

#include "feature.hpp"
#include "feature_impl.hpp"
#include "feature_visibility.hpp"
#include "scales.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/byte_stream.hpp"

#include "../base/logging.hpp"

#include "../base/start_mem_debug.hpp"


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilder1 implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureBuilder1::FeatureBuilder1()
: m_Layer(0), m_bArea(false), m_bHasCenter(false)
{
}

bool FeatureBuilder1::IsGeometryClosed() const
{
  return !m_Geometry.empty() && m_Geometry.front() == m_Geometry.back();
}

void FeatureBuilder1::SetCenter(m2::PointD const & p)
{
  m_Center = p;
  m_bHasCenter = true;
  m_LimitRect.Add(p);
}

void FeatureBuilder1::AddPoint(m2::PointD const & p)
{
  m_Geometry.push_back(p);
  m_LimitRect.Add(p);
}

void FeatureBuilder1::SetAreaAddHoles(list<vector<m2::PointD> > & holes)
{
  m_bArea = true;

  m_Holes.swap(holes);

  for (list<points_t>::iterator i = m_Holes.begin(); i != m_Holes.end();)
  {
    if (i->size() < 3)
      i = m_Holes.erase(i);
    else
      ++i;
  }
}

void FeatureBuilder1::AddName(string const & name)
{
  m_Name = name;
}

void FeatureBuilder1::AddLayer(int32_t layer)
{
  int const bound = 10;
  if (layer < -bound) layer = -bound;
  else if (layer > bound) layer = bound;
  m_Layer = layer;
}

FeatureBase FeatureBuilder1::GetFeatureBase() const
{
  CHECK ( CheckValid(), () );

  FeatureBase f;
  f.SetHeader(GetHeader());

  f.m_Layer = m_Layer;
  for (size_t i = 0; i < m_Types.size(); ++i)
    f.m_Types[i] = m_Types[i];
  f.m_LimitRect = m_LimitRect;
  f.m_Name = m_Name;

  f.m_bTypesParsed = f.m_bLayerParsed = f.m_bNameParsed = f.m_bGeometryParsed = true;

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

bool FeatureBuilder1::operator == (FeatureBuilder1 const & fb) const
{
  if (m_Types != fb.m_Types ||
      m_Layer != fb.m_Layer ||
      m_Name != fb.m_Name ||
      m_bHasCenter != fb.m_bHasCenter ||
      m_bArea != fb.m_bArea)
  {
    return false;
  }

  if (m_bHasCenter && !is_equal(m_Center, fb.m_Center))
    return false;

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
  CHECK(!m_Types.empty() && m_Types.size() <= m_maxTypesCount, ());

  CHECK(m_Layer >= -10 && m_Layer <= 10, ());

  CHECK(m_bHasCenter || m_Geometry.size() >= 2, ());

  CHECK(!m_bArea || m_Geometry.size() >= 3, ());

  CHECK(m_Holes.empty() || m_bArea, ());

  for (list<points_t>::const_iterator i = m_Holes.begin(); i != m_Holes.end(); ++i)
    CHECK(i->size() >= 3, ());

  return true;
}

uint8_t FeatureBuilder1::GetHeader() const
{
  uint8_t header = static_cast<uint8_t>(m_Types.size());

  if (!m_Name.empty())
    header |= FeatureBase::HEADER_HAS_NAME;

  if (m_Layer != 0)
    header |= FeatureBase::HEADER_HAS_LAYER;

  if (m_bHasCenter)
    header |= FeatureBase::HEADER_HAS_POINT;

  size_t const count = m_Geometry.size();

  if (count > 0)
  {
    ASSERT ( count > 1, (count) );
    header |= FeatureBase::HEADER_IS_LINE;

    if (m_bArea)
    {
      ASSERT ( count > 2, (count) );
      header |= FeatureBase::HEADER_IS_AREA;
    }
  }

  return header;
}

void FeatureBuilder1::SerializeBase(buffer_t & data) const
{
  CHECK ( CheckValid(), () );

  PushBackByteSink<buffer_t> sink(data);

  WriteToSink(sink, GetHeader());

  for (size_t i = 0; i < m_Types.size(); ++i)
    WriteVarUint(sink, m_Types[i]);

  if (m_Layer != 0)
    WriteVarInt(sink, m_Layer);

  if (!m_Name.empty())
  {
    WriteVarUint(sink, m_Name.size() - 1);
    sink.Write(&m_Name[0], m_Name.size());
  }

  if (m_bHasCenter)
    WriteVarInt(sink, feature::pts::FromPoint(m_Center));
}

void FeatureBuilder1::Serialize(buffer_t & data) const
{
  data.clear();

  SerializeBase(data);

  PushBackByteSink<buffer_t> sink(data);

  if (!m_Geometry.empty())
    feature::SavePoints(m_Geometry, sink);

  if (m_bArea)
  {
    WriteVarUint(sink, uint32_t(m_Holes.size()));

    for (list<points_t>::const_iterator i = m_Holes.begin(); i != m_Holes.end(); ++i)
      feature::SavePoints(*i, sink);
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
  void CalcRect(vector<m2::PointD> const & points, m2::RectD & rect)
  {
    for (size_t i = 0; i < points.size(); ++i)
      rect.Add(points[i]);
  }
}

void FeatureBuilder1::Deserialize(buffer_t & data)
{
  FeatureBase f;
  f.Deserialize(data, 0);
  f.InitFeatureBuilder(*this);

  ArrayByteSource src(f.DataPtr() + f.m_GeometryOffset);

  FeatureBase::FeatureType const ft = f.GetFeatureType();

  if (ft != FeatureBase::FEATURE_TYPE_POINT)
  {
    feature::LoadPoints(m_Geometry, src);

    CalcRect(m_Geometry, m_LimitRect);
  }

  if (ft == FeatureBase::FEATURE_TYPE_AREA)
  {
    m_bArea = true;

    uint32_t const count = ReadVarUint<uint32_t>(src);
    for (uint32_t i = 0; i < count; ++i)
    {
      m_Holes.push_back(points_t());
      feature::LoadPoints(m_Holes.back(), src);
    }
  }

  CHECK ( CheckValid(), () );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilderGeomRef implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FeatureBuilder2::IsDrawableLikeLine(int lowS, int highS) const
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

void FeatureBuilder2::SerializeOffsets(uint32_t mask, offsets_t const & offsets, buffer_t & buffer)
{
  if (mask > 0)
  {
    PushBackByteSink<buffer_t> sink(buffer);

    WriteVarUint(sink, mask);
    for (size_t i = 0; i < offsets.size(); ++i)
      WriteVarUint(sink, offsets[i]);
  }
}

void FeatureBuilder2::Serialize(buffers_holder_t & data)
{
  data.m_buffer.clear();

  // make flags actual before header serialization
  if (data.m_lineMask == 0)
    m_Geometry.clear();

  if (data.m_trgMask == 0)
    m_bArea = false;

  // header data serialization
  SerializeBase(data.m_buffer);

  // geometry offsets serialization
  SerializeOffsets(data.m_lineMask, data.m_lineOffset, data.m_buffer);
  SerializeOffsets(data.m_trgMask, data.m_trgOffset, data.m_buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(buffer_t & data, uint32_t offset)
{
  m_Offset = offset;
  m_Data.swap(data);

  m_LayerOffset = m_NameOffset = m_CenterOffset = m_GeometryOffset = m_TrianglesOffset = 0;
  m_bTypesParsed = m_bLayerParsed = m_bNameParsed = m_bCenterParsed = m_bGeometryParsed = m_bTrianglesParsed = false;

  m_Layer = 0;
  m_Name.clear();
  m_LimitRect = m2::RectD();
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

void FeatureBase::ParseTypes() const
{
  ASSERT(!m_bTypesParsed, ());

  ArrayByteSource source(DataPtr() + 1);
  for (size_t i = 0; i < GetTypesCount(); ++i)
    m_Types[i] = ReadVarUint<uint32_t>(source);

  m_bTypesParsed = true;
  m_LayerOffset = CalcOffset(source);
}

void FeatureBase::ParseLayer() const
{
  ASSERT(!m_bLayerParsed, ());
  if (!m_bTypesParsed)
    ParseTypes();

  ArrayByteSource source(DataPtr() + m_LayerOffset);
  if (Header() & HEADER_HAS_LAYER)
    m_Layer = ReadVarInt<int32_t>(source);

  m_bLayerParsed = true;
  m_NameOffset = CalcOffset(source);
}

void FeatureBase::ParseName() const
{
  ASSERT(!m_bNameParsed, ());
  if (!m_bLayerParsed)
    ParseLayer();

  ArrayByteSource source(DataPtr() + m_NameOffset);
  if (Header() & HEADER_HAS_NAME)
  {
    m_Name.resize(ReadVarUint<uint32_t>(source) + 1);
    source.Read(&m_Name[0], m_Name.size());
  }

  m_bNameParsed = true;
  m_CenterOffset = CalcOffset(source);
}

void FeatureBase::ParseCenter() const
{
  ASSERT(!m_bCenterParsed, ());
  if (!m_bNameParsed)
    ParseName();

  ArrayByteSource source(DataPtr() + m_CenterOffset);
  if (Header() & HEADER_HAS_POINT)
  {
    m_Center = feature::pts::ToPoint(ReadVarInt<int64_t>(source));
    m_LimitRect.Add(m_Center);
  }

  m_bCenterParsed = true;
  m_GeometryOffset = CalcOffset(source);
}

void FeatureBase::ParseAll() const
{
  if (!m_bCenterParsed)
    ParseCenter();
}

string FeatureBase::DebugString() const
{
  ASSERT(m_bNameParsed, ());

  string res("FEATURE: ");
  res +=  "'" + m_Name + "' ";

  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += "Type:" + debug_print(m_Types[i]) + " ";

  res += "Layer:" + debug_print(m_Layer) + " ";

  if (Header() & HEADER_HAS_POINT)
    res += "Center:" + debug_print(m_Center) + " ";

  return res;
}

void FeatureBase::InitFeatureBuilder(FeatureBuilder1 & fb) const
{
  ParseAll();

  fb.AddTypes(m_Types, m_Types + GetTypesCount());
  fb.AddLayer(m_Layer);
  fb.AddName(m_Name);

  uint8_t const h = Header();

  if (h & HEADER_HAS_POINT)
    fb.SetCenter(m_Center);

  if (h & HEADER_IS_AREA)
    fb.SetAreaAddHoles(list<vector<m2::PointD> >());
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

  m_bOffsetsParsed = false;

  base_type::Deserialize(src.m_data, src.m_offset);
}

uint32_t FeatureType::GetOffset(int scale, offsets_t const & offsets) const
{
  for (size_t i = 0; i < ARRAY_SIZE(feature::g_arrScales); ++i)
    if (scale <= feature::g_arrScales[i])
      return offsets[i];

  return m_invalidOffset;
}

namespace
{
  void Points2String(string & s, vector<m2::PointD> const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
      s += debug_print(points[i]) + " ";
  }
}

string FeatureType::DebugString(int scale) const
{
  // force to load all geometry
  (void)GetLimitRect(scale);

  string s = base_type::DebugString();

  s += "Points:";
  Points2String(s, m_Geometry);

  s += "Triangles:";
  Points2String(s, m_Triangles);

  return s;
}

m2::RectD FeatureType::GetLimitRect(int scale) const
{
  if (!m_bGeometryParsed)
    ParseGeometry(scale);

  if (!m_bTrianglesParsed)
    ParseTriangles(scale);

  if (m_Triangles.empty() && m_Geometry.empty() && (Header() & HEADER_HAS_POINT) == 0)
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume tha rect has zero dimensions.
    m_LimitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_LimitRect;
}

void FeatureType::ParseGeometry(int scale) const
{
  if (!m_bOffsetsParsed)
    ParseOffsets();

  if (Header() & HEADER_IS_LINE)
  {
    uint32_t const offset = GetOffset(scale, m_lineOffsets);
    if (offset != m_invalidOffset)
    {
      ReaderSource<FileReader> src(m_cont->GetReader(feature::GetTagForScale(GEOMETRY_FILE_TAG, scale)));
      src.Skip(offset);
      feature::LoadPoints(m_Geometry, src);

      CalcRect(m_Geometry, m_LimitRect);
    }
  }

  m_bGeometryParsed = true;
}

void FeatureType::ParseTriangles(int scale) const
{
  if (!m_bOffsetsParsed)
    ParseOffsets();

  if (Header() & HEADER_IS_AREA)
  {
    uint32_t const offset = GetOffset(scale, m_trgOffsets);
    if (offset != m_invalidOffset)
    {
      ReaderSource<FileReader> src(m_cont->GetReader(feature::GetTagForScale(TRIANGLE_FILE_TAG, scale)));
      src.Skip(offset);
      feature::LoadTriangles(m_Triangles, src);

      CalcRect(m_Triangles, m_LimitRect);
    }
  }

  m_bTrianglesParsed = true;
}

void FeatureType::ReadOffsetsImpl(ArrayByteSource & src, offsets_t & offsets)
{
  int index = 0;

  uint32_t mask = ReadVarUint<uint32_t>(src);
  ASSERT ( mask > 0, () );
  while (mask > 0)
  {
    ASSERT ( index < ARRAY_SIZE(g_arrScales), (index) );

    offsets[index++] = (mask & 0x01) ? ReadVarUint<uint32_t>(src) : m_invalidOffset;

    mask = mask >> 1;
  }
}

void FeatureType::ParseOffsets() const
{
  if (!m_bCenterParsed)
    ParseCenter();

  ArrayByteSource src(DataPtr() + m_GeometryOffset);

  uint8_t const h = Header();

  if (h & HEADER_IS_LINE)
    ReadOffsetsImpl(src, m_lineOffsets);

  if (h & HEADER_IS_AREA)
    ReadOffsetsImpl(src, m_trgOffsets);

  m_bOffsetsParsed = true;
}
