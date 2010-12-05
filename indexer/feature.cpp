#include "feature.hpp"
#include "../geometry/rect2d.hpp"
#include "../coding/byte_stream.hpp"
#include "../coding/reader.hpp"
#include "../coding/varint.hpp"
#include "../coding/write_to_sink.hpp"
#include "../base/logging.hpp"
#include "../std/bind.hpp"
#include "../std/map.hpp"

namespace pts
{
  inline m2::PointD ToPoint(int64_t i)
  {
    CoordPointT const pt = Int64ToPoint(i);
    return m2::PointD(pt.first, pt.second);
  }
  inline int64_t ToId(CoordPointT const & p)
  {
    return PointToInt64(p.first, p.second);
  }
}

FeatureBuilder::FeatureBuilder() : m_Layer(0)
{
}

bool FeatureBuilder::IsGeometryClosed() const
{
  return !m_Geometry.empty() && m_Geometry.front() == m_Geometry.back();
}

void FeatureBuilder::AddPoint(m2::PointD const & p)
{
  m_Geometry.push_back(pts::ToId(CoordPointT(p.x, p.y)));
}

void FeatureBuilder::AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  m_Triangles.push_back(pts::ToId(CoordPointT(a.x, a.y)));
  m_Triangles.push_back(pts::ToId(CoordPointT(b.x, b.y)));
  m_Triangles.push_back(pts::ToId(CoordPointT(c.x, c.y)));
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

bool FeatureBuilder::operator == (FeatureBuilder const & fb) const
{
  return
      m_Types == fb.m_Types &&
      m_Layer == fb.m_Layer &&
      m_Name == fb.m_Name &&
      m_Geometry == fb.m_Geometry &&
      m_Triangles == fb.m_Triangles;
}

void FeatureBuilder::Serialize(vector<char> & data) const
{
  CHECK(!m_Geometry.empty(), ());
  CHECK(m_Geometry.size() > 1 || m_Triangles.empty(), ());
  CHECK_LESS(m_Types.size(), 16, ());

  data.clear();
  PushBackByteSink<vector<char> > sink(data);

  // Serializing header.
  uint8_t header = static_cast<uint8_t>(m_Types.size());
  if (m_Layer != 0)
    header |= Feature::HEADER_HAS_LAYER;
  if (m_Geometry.size() > 1)
  {
    if (m_Triangles.empty())
      header |= Feature::HEADER_IS_LINE;
    else
      header |= Feature::HEADER_IS_AREA;
  }
  if (!m_Name.empty())
    header |= Feature::HEADER_HAS_NAME;
  WriteToSink(sink, header);

  // Serializing types.
  {
    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, m_Types[i]);
  }

  // Serializing layer.
  if (m_Layer != 0)
    WriteVarInt(sink, m_Layer);

  // Serializing geometry.
  if (m_Geometry.size() == 1)
  {
    WriteVarInt(sink, m_Geometry[0]);
  }
  else
  {
    WriteVarUint(sink, m_Geometry.size() - 1);
    for (size_t i = 0; i < m_Geometry.size(); ++i)
      WriteVarInt(sink, i == 0 ? m_Geometry[0] : m_Geometry[i] - m_Geometry[i-1]);
  }

  // Serializing triangles.
  if (!m_Triangles.empty())
  {
    ASSERT_EQUAL(m_Triangles.size() % 3, 0, (m_Triangles.size()));
    WriteVarUint(sink, m_Triangles.size() / 3 - 1);
    for (size_t i = 0; i < m_Triangles.size(); ++i)
      WriteVarInt(sink, i == 0 ? m_Triangles[i] : (m_Triangles[i] - m_Triangles[i-1]));
  }

  // Serializing name.
  if (!m_Name.empty())
  {
    WriteVarUint(sink, m_Name.size() - 1);
    sink.Write(&m_Name[0], m_Name.size());
  }


#ifdef DEBUG
  vector<char> data1 = data;
  Feature feature;
  feature.DeserializeAndParse(data1);
  FeatureBuilder const fb = feature.GetFeatureBuilder();
  ASSERT_EQUAL(m_Types, fb.m_Types, (feature.DebugString()));
  ASSERT_EQUAL(m_Layer, fb.m_Layer, (feature.DebugString()));
  ASSERT_EQUAL(m_Geometry, fb.m_Geometry, (feature.DebugString()));
  ASSERT_EQUAL(m_Triangles, fb.m_Triangles, (feature.DebugString()));
  ASSERT_EQUAL(m_Name, fb.m_Name, (feature.DebugString()));
  ASSERT(*this == fb, (feature.DebugString()));
#endif
}

Feature FeatureBuilder::GetFeature() const
{
  vector<char> data;
  Serialize(data);
  Feature feature;
  feature.Deserialize(data);
  return feature;
}

Feature::Feature(vector<char> & data, uint32_t offset)
{
  Deserialize(data, offset);
}

void Feature::Deserialize(vector<char> & data, uint32_t offset)
{
  m_Offset = offset;
  m_Data.swap(data);

  m_LayerOffset = m_GeometryOffset = m_TrianglesOffset = m_NameOffset = 0;
  m_bTypesParsed = m_bLayerParsed = m_bGeometryParsed = m_bTrianglesParsed = m_bNameParsed = false;
  m_Layer = m_TriangleCount = 0;
  m_Geometry.clear();
  m_Triangles.clear();
  m_Name.clear();
  m_LimitRect = m2::RectD();
}

void Feature::ParseTypes() const
{
  ASSERT(!m_bTypesParsed, ());
  ArrayByteSource source(DataPtr() + 1);
  for (size_t i = 0; i < GetTypesCount(); ++i)
    m_Types[i] = ReadVarUint<uint32_t>(source);
  m_LayerOffset = static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr());
  m_bTypesParsed = true;
}

void Feature::ParseLayer() const
{
  ASSERT(!m_bLayerParsed, ());
  if (!m_bTypesParsed)
    ParseTypes();

  ArrayByteSource source(DataPtr() + m_LayerOffset);
  if (Header() & HEADER_HAS_LAYER)
    m_Layer = ReadVarInt<int32_t>(source);

  m_GeometryOffset = static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr());
  m_bLayerParsed = true;
}

void Feature::ParseGeometry() const
{
  ASSERT(!m_bGeometryParsed, ());
  if (!m_bLayerParsed)
    ParseLayer();
  ArrayByteSource source(DataPtr() + m_GeometryOffset);
  uint32_t const geometrySize =
      (GetFeatureType() == FEATURE_TYPE_POINT ? 1 : ReadVarUint<uint32_t>(source) + 1);
  m_Geometry.resize(geometrySize);
  int64_t id = 0;
  for (size_t i = 0; i < geometrySize; ++i)
    m_LimitRect.Add(m_Geometry[i] = pts::ToPoint(id += ReadVarInt<int64_t>(source)));
  m_TrianglesOffset = static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr());
  m_bGeometryParsed = true;
}

void Feature::ParseTriangles() const
{
  ASSERT(!m_bTrianglesParsed, ());
  if (!m_bGeometryParsed)
    ParseGeometry();
  ArrayByteSource source(DataPtr() + m_TrianglesOffset);
  if (GetFeatureType() == FEATURE_TYPE_AREA)
  {
    m_TriangleCount = ReadVarUint<uint32_t>(source) + 1;
    uint32_t const trianglePoints = m_TriangleCount * 3;
    m_Triangles.resize(trianglePoints);
    int64_t id = 0;
    for (size_t i = 0; i < trianglePoints; ++i)
      m_Triangles[i] = pts::ToPoint(id += ReadVarInt<int64_t>(source));
  }
  m_NameOffset = static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr());
  m_bTrianglesParsed = true;
}

void Feature::ParseName() const
{
  ASSERT(!m_bNameParsed, ());
  if (!m_bTrianglesParsed)
    ParseTriangles();
  ArrayByteSource source(DataPtr() + m_NameOffset);
  if (Header() & HEADER_HAS_NAME)
  {
    m_Name.resize(ReadVarUint<uint32_t>(source) + 1);
    source.Read(&m_Name[0], m_Name.size());
  }
  m_bNameParsed = true;
  ASSERT_EQUAL(static_cast<uint32_t>(static_cast<char const *>(source.Ptr()) - DataPtr()),
               m_Data.size() - m_Offset, ());
}

void Feature::ParseAll() const
{
  if (!m_bNameParsed)
    ParseName();
}

void Feature::DeserializeAndParse(vector<char> & data, uint32_t offset)
{
  Deserialize(data, offset);
  ParseAll();
}

string Feature::DebugString() const
{
  ParseAll();
  string res("Feature(");
  res +=  "'" + m_Name + "' ";

  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += "Type:" + debug_print(m_Types[i]) + " ";

  res += "Layer:" + debug_print(m_Layer) + " ";
  res += debug_print(m_Geometry) + " ";
  res += debug_print(m_Triangles) + ")";
  return res;
}

FeatureBuilder Feature::GetFeatureBuilder() const
{
  ParseAll();
  FeatureBuilder fb;
  fb.AddTypes(m_Types, m_Types + GetTypesCount());
  fb.AddLayer(m_Layer);
  for (size_t i = 0; i < m_Geometry.size(); ++i)
    fb.AddPoint(m_Geometry[i]);
  ASSERT_EQUAL(m_Triangles.size() % 3, 0, ());
  uint32_t const triangleCount = m_Triangles.size() / 3;
  for (size_t i = 0; i < triangleCount; ++i)
    fb.AddTriangle(m_Triangles[3*i + 0], m_Triangles[3*i + 1], m_Triangles[3*i + 2]);
  fb.AddName(m_Name);
  return fb;
}
