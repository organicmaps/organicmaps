#include "feature.hpp"
#include "cell_id.hpp"

#include "../geometry/rect2d.hpp"

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
// FeatureBuilder implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureBuilder::FeatureBuilder() : m_Layer(0)
{
}

bool FeatureBuilder::IsGeometryClosed() const
{
  return !m_Geometry.empty() && m_Geometry.front() == m_Geometry.back();
}

void FeatureBuilder::AddPoint(m2::PointD const & p)
{
  m_Geometry.push_back(p);
  m_LimitRect.Add(p);
}

void FeatureBuilder::AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  pts::Fpt2id fn;
  m_Triangles.push_back(fn(a));
  m_Triangles.push_back(fn(b));
  m_Triangles.push_back(fn(c));
}

void FeatureBuilder::AddName(string const & name)
{
  CHECK_EQUAL(m_Name, "", (name));
  m_Name = name;
}

void FeatureBuilder::AddLayer(int32_t layer)
{
  CHECK_EQUAL(m_Layer, 0, (layer));

  int const bound = 10;
  if (layer < -bound) layer = -bound;
  else if (layer > bound) layer = bound;
  m_Layer = layer;
}

FeatureBase FeatureBuilder::GetFeatureBase() const
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

bool FeatureBuilder::operator == (FeatureBuilder const & fb) const
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

uint8_t FeatureBuilder::GetHeader() const
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

void FeatureBuilder::Serialize(vector<char> & data) const
{
  CHECK(!m_Geometry.empty(), ());
  CHECK(m_Geometry.size() > 1 || m_Triangles.empty(), ());
  CHECK_LESS(m_Types.size(), 16, ());

  data.clear();
  PushBackByteSink<vector<char> > sink(data);

  // Serializing header.
  WriteToSink(sink, GetHeader());

  // Serializing types.
  {
    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, m_Types[i]);
  }

  // Serializing layer.
  if (m_Layer != 0)
    WriteVarInt(sink, m_Layer);

  // Serializing name.
  if (!m_Name.empty())
  {
    WriteVarUint(sink, m_Name.size() - 1);
    sink.Write(&m_Name[0], m_Name.size());
  }

  size_t const ptsCount = m_Geometry.size();
  vector<int64_t> geom;
  geom.reserve(ptsCount);
  transform(m_Geometry.begin(), m_Geometry.end(), back_inserter(geom), pts::Fpt2id());

  // Serializing geometry.
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

  // Serializing triangles.
  if (!m_Triangles.empty())
  {
    ASSERT_EQUAL(m_Triangles.size() % 3, 0, (m_Triangles.size()));
    WriteVarUint(sink, m_Triangles.size() / 3 - 1);
    for (size_t i = 0; i < m_Triangles.size(); ++i)
      WriteVarInt(sink, i == 0 ? m_Triangles[i] : (m_Triangles[i] - m_Triangles[i-1]));
  }

  ASSERT ( CheckCorrect(data), () );
}

bool FeatureBuilder::CheckCorrect(vector<char> const & data) const
{
  vector<char> data1 = data;
  FeatureGeom f(data1);

  FeatureBuilder fb;
  f.InitFeatureBuilder(fb);

  string const s = f.DebugString();

  ASSERT_EQUAL(m_Layer, f.m_Layer, (s));
  ASSERT_EQUAL(m_Name, f.m_Name, (s));
  ASSERT(is_equal(m_LimitRect, f.m_LimitRect), (s));
  ASSERT(*this == fb, (s));

  return true;
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

void FeatureBase::InitFeatureBuilder(FeatureBuilder & fb) const
{
  ASSERT(m_bNameParsed, ());

  fb.AddTypes(m_Types, m_Types + GetTypesCount());
  fb.AddLayer(m_Layer);
  fb.AddName(m_Name);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureGeom implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureGeom::FeatureGeom(vector<char> & data, uint32_t offset)
{
  Deserialize(data, offset);
}

void FeatureGeom::Deserialize(vector<char> & data, uint32_t offset)
{
  base_type::Deserialize(data, offset);

  m_Geometry.clear();
  m_Triangles.clear();
}

void FeatureGeom::ParseGeometry() const
{
  ASSERT(!m_bGeometryParsed, ());
  if (!m_bNameParsed)
    ParseName();

  ArrayByteSource source(DataPtr() + m_GeometryOffset);
  uint32_t const geometrySize =
    (GetFeatureType() == FEATURE_TYPE_POINT ? 1 : ReadVarUint<uint32_t>(source) + 1);

  m_Geometry.resize(geometrySize);
  int64_t id = 0;
  for (size_t i = 0; i < geometrySize; ++i)
    m_LimitRect.Add(m_Geometry[i] = pts::ToPoint(id += ReadVarInt<int64_t>(source)));

  m_bGeometryParsed = true;
  m_TrianglesOffset = CalcOffset(source);
}

void FeatureGeom::ParseTriangles() const
{
  ASSERT(!m_bTrianglesParsed, ());
  if (!m_bGeometryParsed)
    ParseGeometry();

  ArrayByteSource source(DataPtr() + m_TrianglesOffset);
  if (GetFeatureType() == FEATURE_TYPE_AREA)
  {
    uint32_t const trgPoints = (ReadVarUint<uint32_t>(source) + 1) * 3;
    m_Triangles.resize(trgPoints);
    int64_t id = 0;
    for (size_t i = 0; i < trgPoints; ++i)
      m_Triangles[i] = pts::ToPoint(id += ReadVarInt<int64_t>(source));
  }

  m_bTrianglesParsed = true;
  ASSERT_EQUAL ( CalcOffset(source), m_Data.size() - m_Offset, () );
}

void FeatureGeom::ParseAll() const
{
  if (!m_bTrianglesParsed)
    ParseTriangles();
}

string FeatureGeom::DebugString() const
{
  ParseAll();
  string res = base_type::DebugString();
  res += debug_print(m_Geometry) + " ";
  res += debug_print(m_Triangles) + ")";
  return res;
}

void FeatureGeom::InitFeatureBuilder(FeatureBuilder & fb) const
{
  ParseAll();
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
