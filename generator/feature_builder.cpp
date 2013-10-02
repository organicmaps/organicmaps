#include "feature_builder.hpp"

#include "../indexer/feature_impl.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../indexer/geometry_serialization.hpp"
#include "../indexer/coding_params.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/byte_stream.hpp"

#include "../base/logging.hpp"


using namespace feature;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilder1 implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureBuilder1::FeatureBuilder1()
: m_coastCell(-1)
{
  m_Polygons.push_back(points_t());
}

bool FeatureBuilder1::IsGeometryClosed() const
{
  points_t const & poly = GetGeometry();
  return (poly.size() > 2 && poly.front() == poly.back());
}

m2::PointD FeatureBuilder1::GetGeometryCenter() const
{
  ASSERT ( IsGeometryClosed(), () );
  m2::PointD ret(0.0, 0.0);

  points_t const & poly = GetGeometry();
  size_t const count = poly.size();
  for (size_t i = 0; i < count; ++i)
    ret += poly[i];
  return ret / count;
}

void FeatureBuilder1::SetCenter(m2::PointD const & p)
{
  m_Center = p;
  m_Params.SetGeomType(GEOM_POINT);
  m_LimitRect.Add(p);
}

void FeatureBuilder1::AddPoint(m2::PointD const & p)
{
  m_Polygons.front().push_back(p);
  m_LimitRect.Add(p);
}

void FeatureBuilder1::SetAreaAddHoles(list<points_t> const & holes)
{
  m_Params.SetGeomType(GEOM_AREA);
  m_Polygons.resize(1);

  if (holes.empty()) return;

  points_t const & poly = GetGeometry();
  m2::Region<m2::PointD> rgn(poly.begin(), poly.end());

  for (list<points_t>::const_iterator i = holes.begin(); i != holes.end(); ++i)
  {
    ASSERT ( !i->empty(), (*this) );

    size_t j = 0;
    size_t const count = i->size();
    for (; j < count; ++j)
      if (!rgn.Contains((*i)[j]))
        break;

    if (j == count)
      m_Polygons.push_back(*i);
  }
}

void FeatureBuilder1::AddPolygon(vector<m2::PointD> & poly)
{
  // check for closing
  if (poly.size() < 3) return;
  if (poly.front() != poly.back())
    poly.push_back(poly.front());

  CalcRect(poly, m_LimitRect);

  if (!m_Polygons.back().empty())
    m_Polygons.push_back(points_t());

  m_Polygons.back().swap(poly);
}

void FeatureBuilder1::DoCorrectForType(EGeomType type)
{
  if (m_Params.GetGeomType() == type &&
      !IsDrawableLike(m_Params.m_Types, static_cast<FeatureGeoType>(type)))
  {
    m_Params.RemoveGeomType(type);
  }
}

bool FeatureBuilder1::DoCorrect()
{
  DoCorrectForType(GEOM_AREA);
  DoCorrectForType(GEOM_LINE);

  return (m_Params.GetGeomType() != GEOM_UNDEFINED);
}

FeatureBase FeatureBuilder1::GetFeatureBase() const
{
  CHECK ( CheckValid(), (*this) );

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
  static const int8_t defaultCode = StringUtf8Multilang::GetLangIndex("default");
  static const int8_t intCode = StringUtf8Multilang::GetLangIndex("int_name");

  switch (m_Params.GetGeomType())
  {
  case GEOM_POINT:
    // If we don't have name and have house number, than replace them.
    if (m_Params.name.IsEmpty() && !m_Params.house.IsEmpty())
      m_Params.name.AddString(defaultCode, m_Params.house.Get());

    // We need refs for motorway's junctions. Try to update name always, not only for junctions.
    if (m_Params.name.IsEmpty() && (!m_Params.ref.empty() || m_Params.flats.empty()))
    {
      m_Params.name.AddString(defaultCode, m_Params.ref);
      m_Params.name.AddString(intCode, m_Params.flats);
    }

    m_Params.house.Clear();
    m_Params.ref.clear();
    m_Params.flats.clear();
    break;

  case GEOM_LINE:
  {
    static feature::TypeSetChecker checkHighway("highway");

    // We need refs for road's numbers.
    if (!checkHighway.IsEqualV(m_Params.m_Types))
      m_Params.ref = string();

    m_Params.rank = 0;
    m_Params.house.Clear();
    break;
  }

  case GEOM_AREA:
    m_Params.rank = 0;
    m_Params.ref.clear();
    break;

  default:
    return false;
  }

  // Clear name for features with invisible texts.
  // AlexZ: Commented this line to enable captions on subway exits, which
  // are not drawn but should be visible in balloons and search results
  //RemoveNameIfInvisible();

  return true;
}

void FeatureBuilder1::RemoveNameIfInvisible(int minS, int maxS)
{
  int64_t dummy;
  if (!m_Params.name.IsEmpty() && !GetCoastCell(dummy))
  {
    pair<int, int> const range = GetDrawableScaleRangeForRules(GetFeatureBase(), RULE_ANY_TEXT);
    if (range.first > maxS || range.second < minS)
      m_Params.name.Clear();
  }
}

bool FeatureBuilder1::operator == (FeatureBuilder1 const & fb) const
{
  if (!(m_Params == fb.m_Params)) return false;

  if (m_coastCell != fb.m_coastCell) return false;

  if (m_Params.GetGeomType() == GEOM_POINT &&
      !is_equal(m_Center, fb.m_Center))
  {
    return false;
  }

  if (!is_equal(m_LimitRect, fb.m_LimitRect))
  {
    //LOG(LERROR, ("Different rects: ", m_LimitRect, fb.m_LimitRect));
    return false;
  }

  if (m_Polygons.size() != fb.m_Polygons.size())
    return false;

  list<points_t>::const_iterator i = m_Polygons.begin();
  list<points_t>::const_iterator j = fb.m_Polygons.begin();
  for (; i != m_Polygons.end(); ++i, ++j)
    if (!is_equal(*i, *j))
    {
      //LOG(LERROR, ("Different points: ", *i, *j));
      return false;
    }

  return true;
}

bool FeatureBuilder1::CheckValid() const
{
  CHECK(m_Params.CheckValid(), (*this));

  EGeomType const type = m_Params.GetGeomType();
  CHECK ( type != GEOM_UNDEFINED, (*this) );

  points_t const & poly = GetGeometry();

  if (type == GEOM_LINE)
    CHECK(poly.size() >= 2, (*this));

  if (type == GEOM_AREA)
  {
    for (list<points_t>::const_iterator i = m_Polygons.begin(); i != m_Polygons.end(); ++i)
      CHECK(i->size() >= 3, (*this));
  }

  return true;
}

void FeatureBuilder1::SerializeBase(buffer_t & data, serial::CodingParams const & params) const
{
  PushBackByteSink<buffer_t> sink(data);

  m_Params.Write(sink);

  if (m_Params.GetGeomType() == GEOM_POINT)
    serial::SavePoint(sink, m_Center, params);
}

void FeatureBuilder1::Serialize(buffer_t & data) const
{
  CHECK ( CheckValid(), (*this) );

  data.clear();

  serial::CodingParams cp;

  SerializeBase(data, cp);

  PushBackByteSink<buffer_t> sink(data);

  if (m_Params.GetGeomType() != GEOM_POINT)
  {
    WriteVarUint(sink, static_cast<uint32_t>(m_Polygons.size()));

    for (list<points_t>::const_iterator i = m_Polygons.begin(); i != m_Polygons.end(); ++i)
      serial::SaveOuterPath(*i, cp, sink);
  }

  WriteVarInt(sink, m_coastCell);

  // check for correct serialization
#ifdef DEBUG
  buffer_t tmp(data);
  FeatureBuilder1 fb;
  fb.Deserialize(tmp);
  ASSERT ( fb == *this, ("Source feature: ", *this, "Deserialized feature: ", fb) );
#endif
}

void FeatureBuilder1::Deserialize(buffer_t & data)
{
  serial::CodingParams cp;

  ArrayByteSource source(&data[0]);
  m_Params.Read(source);

  m_LimitRect.MakeEmpty();

  EGeomType const type = m_Params.GetGeomType();
  if (type == GEOM_POINT)
  {
    m_Center = serial::LoadPoint(source, cp);
    m_LimitRect.Add(m_Center);
    return;
  }

  m_Polygons.clear();
  uint32_t const count = ReadVarUint<uint32_t>(source);
  ASSERT_GREATER ( count, 0, (*this) );

  for (uint32_t i = 0; i < count; ++i)
  {
    m_Polygons.push_back(points_t());
    serial::LoadOuterPath(source, cp, m_Polygons.back());
    CalcRect(m_Polygons.back(), m_LimitRect);
  }

  m_coastCell = ReadVarInt<int64_t>(source);

  CHECK ( CheckValid(), (*this) );
}

void FeatureBuilder1::AddOsmId(string const & type, uint64_t osmId)
{
  m_osmIds.push_back(osm::OsmId(type, osmId));
}

string FeatureBuilder1::GetOsmIdsString() const
{
  ostringstream out;
  for (size_t i = 0; i < m_osmIds.size(); ++i)
    out << m_osmIds[i].Type() << " id=" << m_osmIds[i].Id() << " ";
  return out.str();
}

int FeatureBuilder1::GetMinFeatureDrawScale() const
{
  int const minScale = feature::GetMinDrawableScale(GetFeatureBase());

  // some features become invisible after merge processing, so -1 is possible
  return (minScale == -1 ? 1000 : minScale);
}

void FeatureBuilder1::SetCoastCell(int64_t iCell, string const & strCell)
{
  m_coastCell = iCell;

  ASSERT ( m_Params.name.IsEmpty(), () );
  m_Params.name.AddString(0, strCell);
}

string DebugPrint(FeatureBuilder1 const & f)
{
  ostringstream out;

  switch (f.GetGeomType())
  {
  case GEOM_POINT: out << DebugPrint(f.m_Center); break;
  case GEOM_LINE: out << "line with " << f.GetPointsCount() << " points"; break;
  case GEOM_AREA: out << "area with " << f.GetPointsCount() << " points"; break;
  default:
    out << "ERROR: unknown geometry type"; break;
  }

  return (out.str() + " " +
          DebugPrint(f.m_LimitRect) + " " +
          DebugPrint(f.m_Params) + " " +
          DebugPrint(f.m_Polygons)
          );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilderGeomRef implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FeatureBuilder2::IsDrawableInRange(int lowS, int highS) const
{
  if (!GetGeometry().empty())
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
      ASSERT_GREATER ( GetGeometry().size(), 2, () );

      // Store first point once for outer linear features.
      serial::SavePoint(sink, GetGeometry()[0], params);

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
