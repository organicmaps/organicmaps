#include "feature.hpp"
#include "cell_id.hpp"
#include "feature_visibility.hpp"
#include "scales.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/distance.hpp"
#include "../geometry/simplification.hpp"

#include "../coding/byte_stream.hpp"
#include "../coding/reader.hpp"
#include "../coding/varint.hpp"
#include "../coding/write_to_sink.hpp"

#include "../base/logging.hpp"

#include "../base/start_mem_debug.hpp"


namespace pts
{
  inline m2::PointD ToPoint(int64_t i)
  {
    CoordPointT const pt = Int64ToPoint(i);
    return m2::PointD(pt.first, pt.second);
  }

  struct Fpt2id
  {
    int64_t operator() (m2::PointD const & p)
    {
      return PointToInt64(p.x, p.y);
    }
  };
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilderGeom implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureBuilderGeom::FeatureBuilderGeom() : m_Layer(0)
{
}

bool FeatureBuilderGeom::IsGeometryClosed() const
{
  return !m_Geometry.empty() && m_Geometry.front() == m_Geometry.back();
}

void FeatureBuilderGeom::AddPoint(m2::PointD const & p)
{
  m_Geometry.push_back(p);
  m_LimitRect.Add(p);
}

void FeatureBuilderGeom::AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  pts::Fpt2id fn;
  m_Triangles.push_back(fn(a));
  m_Triangles.push_back(fn(b));
  m_Triangles.push_back(fn(c));
}

void FeatureBuilderGeom::AddName(string const & name)
{
  CHECK_EQUAL(m_Name, "", (name));
  m_Name = name;
}

void FeatureBuilderGeom::AddLayer(int32_t layer)
{
  CHECK_EQUAL(m_Layer, 0, (layer));

  int const bound = 10;
  if (layer < -bound) layer = -bound;
  else if (layer > bound) layer = bound;
  m_Layer = layer;
}

FeatureBase FeatureBuilderGeom::GetFeatureBase() const
{
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
  bool is_equal(m2::RectD const & r1, m2::RectD const & r2)
  {
    return (is_equal(r1.minX(), r2.minX()) &&
            is_equal(r1.minY(), r2.minY()) &&
            is_equal(r1.maxX(), r2.maxX()) &&
            is_equal(r1.maxY(), r2.maxY()));
  }
}

bool FeatureBuilderGeom::operator == (FeatureBuilderGeom const & fb) const
{
  if (m_Geometry.size() != fb.m_Geometry.size())
    return false;

  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  for (size_t i = 0; i < m_Geometry.size(); ++i)
    if (!m_Geometry[i].EqualDxDy(fb.m_Geometry[i], eps))
      return false;

  return
      m_Types == fb.m_Types &&
      m_Layer == fb.m_Layer &&
      m_Name == fb.m_Name &&
      m_Triangles == fb.m_Triangles &&
      is_equal(m_LimitRect, fb.m_LimitRect);
}

uint8_t FeatureBuilderGeom::GetHeader() const
{
  uint8_t header = static_cast<uint8_t>(m_Types.size());
  if (m_Layer != 0)
    header |= FeatureBase::HEADER_HAS_LAYER;
  if (m_Geometry.size() > 1)
  {
    if (m_Triangles.empty())
      header |= FeatureBase::HEADER_IS_LINE;
    else
      header |= FeatureBase::HEADER_IS_AREA;
  }
  if (!m_Name.empty())
    header |= FeatureBase::HEADER_HAS_NAME;
  return header;
}

void FeatureBuilderGeom::SerializeBase(buffer_t & data) const
{
  CHECK(!m_Geometry.empty(), ());
  CHECK(m_Geometry.size() > 1 || m_Triangles.empty(), ());
  CHECK_LESS(m_Types.size(), 16, ());

  PushBackByteSink<buffer_t> sink(data);

  // Serialize header.
  WriteToSink(sink, GetHeader());

  // Serialize types.
  {
    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, m_Types[i]);
  }

  // Serialize layer.
  if (m_Layer != 0)
    WriteVarInt(sink, m_Layer);

  // Serialize name.
  if (!m_Name.empty())
  {
    WriteVarUint(sink, m_Name.size() - 1);
    sink.Write(&m_Name[0], m_Name.size());
  }
}

void FeatureBuilderGeom::Serialize(buffers_holder_t & data) const
{
  data.clear();

  SerializeBase(data);
  SerializePoints(m_Geometry, data);
  SerializeTriangles(data);

  ASSERT ( CheckCorrect(data), () );
}

void FeatureBuilderGeom::SerializeTriangles(buffer_t & data) const
{
  if (!m_Triangles.empty())
  {
    PushBackByteSink<buffer_t> sink(data);

    ASSERT_EQUAL(m_Triangles.size() % 3, 0, (m_Triangles.size()));
    WriteVarUint(sink, m_Triangles.size() / 3 - 1);
    for (size_t i = 0; i < m_Triangles.size(); ++i)
      WriteVarInt(sink, i == 0 ? m_Triangles[i] : (m_Triangles[i] - m_Triangles[i-1]));
  }
}

void FeatureBuilderGeom::SerializePoints(points_t const & points, buffer_t & data)
{
  uint32_t const ptsCount = points.size();
  ASSERT_GREATER_OR_EQUAL(ptsCount, 1, ());

  vector<int64_t> geom;
  geom.reserve(ptsCount);
  transform(points.begin(), points.end(), back_inserter(geom), pts::Fpt2id());

  PushBackByteSink<buffer_t> sink(data);

  if (ptsCount == 1)
  {
    WriteVarInt(sink, geom[0]);
  }
  else
  {
    WriteVarUint(sink, ptsCount - 1);
    for (size_t i = 0; i < ptsCount; ++i)
      WriteVarInt(sink, i == 0 ? geom[0] : geom[i] - geom[i-1]);
  }
}

bool FeatureBuilderGeom::CheckCorrect(vector<char> const & data) const
{
  FeatureGeom::read_source_t src;
  src.m_data = data;
  FeatureGeom f(src);

  FeatureBuilderGeom fb;
  f.InitFeatureBuilder(fb);

  string const s = f.DebugString();

  ASSERT_EQUAL(m_Layer, f.m_Layer, (s));
  ASSERT_EQUAL(m_Name, f.m_Name, (s));
  ASSERT(is_equal(m_LimitRect, f.m_LimitRect), (s));
  ASSERT(*this == fb, (s));

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilderGeomRef implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FeatureBuilderGeomRef::IsDrawable(int lowS, int highS) const
{
  FeatureBase const fb = GetFeatureBase();

  while (lowS <= highS)
    if (feature::IsDrawableForIndex(fb, lowS++))
      return true;

  return false;
}

void FeatureBuilderGeomRef::SimplifyPoints(points_t const & in, points_t & out, int level) const
{
  if (in.size() >= 2)
  {
    SimplifyNearOptimal<mn::DistanceToLineSquare<m2::PointD> >(50, in.begin(), in.end()-1,
      my::sq(scales::GetEpsilonForLevel(level + 1)), MakeBackInsertFunctor(out));

    switch (out.size())
    {
    case 0:
      out.push_back(in.front());
    case 1:
      out.push_back(in.back());
      break;
    default:
      if (!out.back().EqualDxDy(in.back(), MercatorBounds::GetCellID2PointAbsEpsilon()))
        out.push_back(in.back());
    }
  }
}

int g_arrScales[] = { 5, 10, 14, 17 };  // 17 = scales::GetUpperScale()
//int g_arrScales[] = { 17 };

void FeatureBuilderGeomRef::Serialize(buffers_holder_t & data) const
{
  data.clear();

  SerializeBase(data.m_buffers[0]);

  PushBackByteSink<buffer_t> sink(data.m_buffers[0]);

  // for point feature write geometry-point immediately
  if (m_Geometry.size() == 1)
  {
    SerializePoints(m_Geometry, data.m_buffers[0]);
  }
  else
  {
    uint32_t mask = 0;
    int lowS = 0;
    vector<uint32_t> offsets;
    for (int i = 0; i < ARRAY_SIZE(g_arrScales); ++i)
    {
      if (IsDrawable(lowS, g_arrScales[i]))
      {
        mask |= (1 << i);
        offsets.push_back(data.m_buffers[1].size());

        // serialize points
        points_t points;
        SimplifyPoints(m_Geometry, points, g_arrScales[i]);
        SerializePoints(points, data.m_buffers[1]);
      }
      lowS = g_arrScales[i]+1;
    }

    CHECK(mask > 0, (mask));  // feature should be visible

    // serialize geometry offsets
    WriteVarUint(sink, mask);
    for (size_t i = 0; i < offsets.size(); ++i)
      WriteVarUint(sink, data.m_lineOffset + offsets[i]);
  }

  if (!m_Triangles.empty())
  {
    WriteVarUint(sink, data.m_trgOffset);
    SerializeTriangles(data.m_buffers[2]);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(vector<char> & data, uint32_t offset)
{
  m_Offset = offset;
  m_Data.swap(data);

  m_LayerOffset = m_GeometryOffset = m_TrianglesOffset = m_NameOffset = 0;
  m_bTypesParsed = m_bLayerParsed = m_bGeometryParsed = m_bTrianglesParsed = m_bNameParsed = false;

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
  m_GeometryOffset = CalcOffset(source);
}

string FeatureBase::DebugString() const
{
  ASSERT(m_bNameParsed, ());

  string res("Feature(");
  res +=  "'" + m_Name + "' ";

  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += "Type:" + debug_print(m_Types[i]) + " ";

  res += "Layer:" + debug_print(m_Layer) + " ";
  return res;
}

void FeatureBase::InitFeatureBuilder(FeatureBuilderGeom & fb) const
{
  ASSERT(m_bNameParsed, ());

  fb.AddTypes(m_Types, m_Types + GetTypesCount());
  fb.AddLayer(m_Layer);
  fb.AddName(m_Name);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureGeom implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureGeom::FeatureGeom(read_source_t & src)
{
  Deserialize(src);
}

void FeatureGeom::Deserialize(read_source_t & src)
{
  base_type::Deserialize(src.m_data, src.m_offset);

  m_Geometry.clear();
  m_Triangles.clear();
}

template <class TSource> 
void FeatureGeom::ParseGeometryImpl(TSource & src) const
{
  ASSERT(!m_bGeometryParsed, ());
  uint32_t const geometrySize =
    (GetFeatureType() == FEATURE_TYPE_POINT ? 1 : ReadVarUint<uint32_t>(src) + 1);

  m_Geometry.resize(geometrySize);
  int64_t id = 0;
  for (size_t i = 0; i < geometrySize; ++i)
    m_LimitRect.Add(m_Geometry[i] = pts::ToPoint(id += ReadVarInt<int64_t>(src)));

  m_bGeometryParsed = true;
}

void FeatureGeom::ParseGeometry(int) const
{
  if (!m_bNameParsed)
    ParseName();

  ArrayByteSource source(DataPtr() + m_GeometryOffset);
  ParseGeometryImpl(source);

  m_TrianglesOffset = CalcOffset(source);
}

template <class TSource> 
void FeatureGeom::ParseTrianglesImpl(TSource & src) const
{
  ASSERT(!m_bTrianglesParsed, ());
  if (GetFeatureType() == FEATURE_TYPE_AREA)
  {
    uint32_t const trgPoints = (ReadVarUint<uint32_t>(src) + 1) * 3;
    m_Triangles.resize(trgPoints);
    int64_t id = 0;
    for (size_t i = 0; i < trgPoints; ++i)
      m_Triangles[i] = pts::ToPoint(id += ReadVarInt<int64_t>(src));
  }
  m_bTrianglesParsed = true;
}

void FeatureGeom::ParseTriangles(int scale) const
{
  if (!m_bGeometryParsed)
    ParseGeometry(scale);

  ArrayByteSource source(DataPtr() + m_TrianglesOffset);
  ParseTrianglesImpl(source);

  ASSERT_EQUAL ( CalcOffset(source), m_Data.size() - m_Offset, () );
}

void FeatureGeom::ParseAll(int scale) const
{
  if (!m_bGeometryParsed)
    ParseGeometry(scale);

  if (!m_bTrianglesParsed)
    ParseTriangles(scale);
}

string FeatureGeom::DebugString(int scale) const
{
  ParseAll(scale);
  string res = base_type::DebugString();
  res += debug_print(m_Geometry) + " ";
  res += debug_print(m_Triangles) + ")";
  return res;
}

string FeatureGeom::DebugString() const
{
  string ret = DebugString(m_defScale);
  ASSERT ( !ret.empty(), () );
  return ret;
}

void FeatureGeom::InitFeatureBuilder(FeatureBuilderGeom & fb) const
{
  ParseAll(m_defScale);
  base_type::InitFeatureBuilder(fb);

  for (size_t i = 0; i < m_Geometry.size(); ++i)
    fb.AddPoint(m_Geometry[i]);

  ASSERT_EQUAL(m_Triangles.size() % 3, 0, ());
  uint32_t const triangleCount = m_Triangles.size() / 3;
  for (size_t i = 0; i < triangleCount; ++i)
    fb.AddTriangle(m_Triangles[3*i + 0], m_Triangles[3*i + 1], m_Triangles[3*i + 2]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureGeomRef implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureGeomRef::FeatureGeomRef(read_source_t & src)
 : base_type(src), m_gF(&src.m_gF), m_trgF(&src.m_trgF)
{
}

void FeatureGeomRef::Deserialize(read_source_t & src)
{
  m_gF = &src.m_gF;
  m_trgF = &src.m_trgF;

  m_bOffsetsParsed = false;
  m_mask = 0;
  m_gOffsets.clear();

  base_type::Deserialize(src);
}

uint32_t FeatureGeomRef::GetOffset(int scale) const
{
  for (size_t i = 0; i < ARRAY_SIZE(g_arrScales); ++i)
    if (scale <= g_arrScales[i])
      return m_gOffsets[i];

  return m_invalidOffset;
}

string FeatureGeomRef::DebugString(int scale) const
{
  if (!m_bOffsetsParsed)
    ParseOffsets();

  if (!m_bGeometryParsed && GetOffset(scale) == m_invalidOffset)
    return string();
  else
    return base_type::DebugString(scale);
}

void FeatureGeomRef::ParseGeometry(int scale) const
{
  if (!m_bOffsetsParsed)
    ParseOffsets();

  if (m_bGeometryParsed)
    return;

  ReaderSource<FileReader> source(*m_gF);
  uint32_t const offset = GetOffset(scale);
  CHECK ( offset != m_invalidOffset, (offset) );
  source.Skip(offset);

  ParseGeometryImpl(source);
}

void FeatureGeomRef::ParseTriangles(int scale) const
{
  if (!m_bOffsetsParsed)
    ParseOffsets();

  ReaderSource<FileReader> source(*m_trgF);
  source.Skip(m_trgOffset);

  ParseTrianglesImpl(source);
}

void FeatureGeomRef::ParseOffsets() const
{
  if (!m_bNameParsed)
    ParseName();

  ArrayByteSource source(DataPtr() + m_GeometryOffset);
  FeatureType const type = GetFeatureType();

  if (type == FEATURE_TYPE_POINT)
  {
    ParseGeometryImpl(source);
  }
  else
  {
    uint32_t mask = m_mask = ReadVarUint<uint32_t>(source);
    ASSERT ( mask > 0, () );
    while (mask > 0)
    {
      if (mask & 0x01)
        m_gOffsets.push_back(ReadVarUint<uint32_t>(source));
      else
        m_gOffsets.push_back((uint32_t)m_invalidOffset);

      mask = mask >> 1;
    }

    if (type == FEATURE_TYPE_AREA)
      m_trgOffset = ReadVarUint<uint32_t>(source);
  }

  m_bOffsetsParsed = true;
}
