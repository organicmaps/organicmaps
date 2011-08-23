#include "../base/SRC_FIRST.hpp"

#include "feature_builder.hpp"

#include "../indexer/feature_impl.hpp"
#include "../indexer/feature_visibility.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/byte_stream.hpp"

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
  if (!m_Params.name.IsEmpty() && feature::DrawableScaleRangeForText(GetFeatureBase()).first == -1)
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

void FeatureBuilder1::Deserialize(buffer_t & data)
{
  FeatureBase f;
  f.Deserialize(data, 0, serial::CodingParams());
  InitFeatureBuilder(f);

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

void FeatureBuilder1::InitFeatureBuilder(FeatureBase const & ft)
{
  ft.ParseAll();

  m_Params = ft.GetFeatureParams();

  if (ft.GetFeatureType() == GEOM_POINT)
  {
    SetCenter(ft.GetCenter());
    m_Params.SetGeomType(GEOM_POINT);
  }
}
