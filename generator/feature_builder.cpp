#include "generator/feature_builder.hpp"

#include "routing/routing_helpers.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_visibility.hpp"

#include "coding/bit_streams.hpp"
#include "coding/byte_stream.hpp"
#include "coding/geometry_coding.hpp"

#include "geometry/region2d.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

using namespace feature;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBuilder1 implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureBuilder1::FeatureBuilder1()
  : m_coastCell(-1)
{
  m_polygons.push_back(PointSeq());
}

bool FeatureBuilder1::IsGeometryClosed() const
{
  PointSeq const & poly = GetOuterGeometry();
  return (poly.size() > 2 && poly.front() == poly.back());
}

m2::PointD FeatureBuilder1::GetGeometryCenter() const
{
  //TODO(vng): Check requirements in this assert
  //ASSERT ( IsGeometryClosed(), () );
  m2::PointD ret(0.0, 0.0);

  PointSeq const & poly = GetOuterGeometry();
  size_t const count = poly.size();
  for (size_t i = 0; i < count; ++i)
    ret += poly[i];
  return ret / count;
}

m2::PointD FeatureBuilder1::GetKeyPoint() const
{
  switch (GetGeomType())
  {
  case GEOM_POINT:
    return m_center;
  case GEOM_LINE: case GEOM_AREA:
    return GetGeometryCenter();
  default:
    CHECK(false, ());
    return m2::PointD();
  }
}

void FeatureBuilder1::SetCenter(m2::PointD const & p)
{
  m_center = p;
  m_params.SetGeomType(GEOM_POINT);
  m_limitRect.Add(p);
}

void FeatureBuilder1::SetRank(uint8_t rank)
{
  m_params.rank = rank;
}

void FeatureBuilder1::AddHouseNumber(string const & houseNumber)
{
  m_params.AddHouseNumber(houseNumber);
}

void FeatureBuilder1::AddStreet(string const & streetName) { m_params.AddStreet(streetName); }

void FeatureBuilder1::AddPostcode(string const & postcode)
{
  m_params.GetMetadata().Set(Metadata::FMD_POSTCODE, postcode);
}

void FeatureBuilder1::AddPoint(m2::PointD const & p)
{
  m_polygons.front().push_back(p);
  m_limitRect.Add(p);
}

void FeatureBuilder1::SetLinear(bool reverseGeometry)
{
  m_params.SetGeomType(feature::GEOM_LINE);
  m_polygons.resize(1);

  if (reverseGeometry)
  {
    auto & cont = m_polygons.front();
    ASSERT(!cont.empty(), ());
    reverse(cont.begin(), cont.end());
  }
}

void FeatureBuilder1::SetAreaAddHoles(FeatureBuilder1::Geometry const & holes)
{
  m_params.SetGeomType(GEOM_AREA);
  m_polygons.resize(1);

  if (holes.empty()) return;

  PointSeq const & poly = GetOuterGeometry();
  m2::Region<m2::PointD> rgn(poly.begin(), poly.end());

  for (PointSeq const & points : holes)
  {
    ASSERT ( !points.empty(), (*this) );

    size_t j = 0;
    size_t const count = points.size();
    for (; j < count; ++j)
      if (!rgn.Contains(points[j]))
        break;

    if (j == count)
      m_polygons.push_back(points);
  }
}

void FeatureBuilder1::AddPolygon(vector<m2::PointD> & poly)
{
  // check for closing
  if (poly.size() < 3)
    return;

  if (poly.front() != poly.back())
    poly.push_back(poly.front());

  CalcRect(poly, m_limitRect);

  if (!m_polygons.back().empty())
    m_polygons.push_back(PointSeq());

  m_polygons.back().swap(poly);
}

void FeatureBuilder1::ResetGeometry()
{
  m_polygons.clear();
  m_polygons.push_back(PointSeq());
  m_limitRect.MakeEmpty();
}

bool FeatureBuilder1::RemoveInvalidTypes()
{
  if (!m_params.FinishAddingTypes())
    return false;

  return feature::RemoveUselessTypes(m_params.m_types, m_params.GetGeomType(),
                                     m_params.IsEmptyNames());
}

bool FeatureBuilder1::FormatFullAddress(string & res) const
{
  return m_params.FormatFullAddress(m_limitRect.Center(), res);
}

TypesHolder FeatureBuilder1::GetTypesHolder() const
{
  CHECK ( CheckValid(), (*this) );

  TypesHolder holder(m_params.GetGeomType());
  for (auto const t : m_params.m_types)
    holder.Add(t);

  return holder;
}

namespace
{
bool IsEqual(double d1, double d2)
{
  return base::AlmostEqualAbs(d1, d2, kMwmPointAccuracy);
}

bool IsEqual(m2::PointD const & p1, m2::PointD const & p2)
{
  return p1.EqualDxDy(p2, kMwmPointAccuracy);
}

bool IsEqual(m2::RectD const & r1, m2::RectD const & r2)
{
  return (IsEqual(r1.minX(), r2.minX()) &&
          IsEqual(r1.minY(), r2.minY()) &&
          IsEqual(r1.maxX(), r2.maxX()) &&
          IsEqual(r1.maxY(), r2.maxY()));
}

bool IsEqual(vector<m2::PointD> const & v1, vector<m2::PointD> const & v2)
{
  return equal(cbegin(v1), cend(v1), cbegin(v2), cend(v2),
               [](m2::PointD const & p1, m2::PointD const & p2) { return IsEqual(p1, p2); });
}
}

bool FeatureBuilder1::IsRoad() const
{
  return routing::IsRoad(m_params.m_types);
}

bool FeatureBuilder1::PreSerialize()
{
  if (!m_params.IsValid())
    return false;

  switch (m_params.GetGeomType())
  {
  case GEOM_POINT:
    // Store house number like HEADER_GEOM_POINT_EX.
    if (!m_params.house.IsEmpty())
    {
      m_params.SetGeomTypePointEx();
      m_params.rank = 0;
    }

    // Store ref's in name field (used in "highway-motorway_junction").
    if (m_params.name.IsEmpty() && !m_params.ref.empty())
      m_params.name.AddString(StringUtf8Multilang::kDefaultCode, m_params.ref);

    m_params.ref.clear();
    break;

  case GEOM_LINE:
  {
    // We need refs for road's numbers.
    if (!IsRoad())
      m_params.ref.clear();

    m_params.rank = 0;
    m_params.house.Clear();
    break;
  }

  case GEOM_AREA:
    m_params.rank = 0;
    m_params.ref.clear();
    break;

  default:
    return false;
  }

  return true;
}

bool FeatureBuilder1::PreSerializeAndRemoveUselessNames()
{
  if (!PreSerialize())
    return false;

  // Clear name for features with invisible texts.
  // AlexZ: Commented this line to enable captions on subway exits, which
  // are not drawn but should be visible in balloons and search results
  //RemoveNameIfInvisible();
  RemoveUselessNames();

  return true;
}

void FeatureBuilder1::RemoveUselessNames()
{
  if (!m_params.name.IsEmpty() && !IsCoastCell())
  {
    using namespace feature;
    // Use lambda syntax to correctly compile according to standard:
    // http://en.cppreference.com/w/cpp/algorithm/remove
    //     The signature of the predicate function should be equivalent to the following:
    //     bool pred(const Type &a);
    // Without it on clang-libc++ on Linux we get:
    // candidate template ignored: substitution failure
    //      [with _Tp = bool (unsigned int) const]: reference to function type 'bool (unsigned int) const' cannot have 'const'
    //      qualifier
    auto const typeRemover = [](uint32_t type)
    {
      static TypeSetChecker const checkBoundary({ "boundary", "administrative" });
      return checkBoundary.IsEqual(type);
    };

    auto types = GetTypesHolder();
    if (types.RemoveIf(typeRemover))
    {
      pair<int, int> const range = GetDrawableScaleRangeForRules(types, RULE_ANY_TEXT);
      if (range.first == -1)
        m_params.name.Clear();
    }
  }
}

void FeatureBuilder1::RemoveNameIfInvisible(int minS, int maxS)
{
  if (!m_params.name.IsEmpty() && !IsCoastCell())
  {
    pair<int, int> const range = GetDrawableScaleRangeForRules(GetTypesHolder(), RULE_ANY_TEXT);
    if (range.first > maxS || range.second < minS)
      m_params.name.Clear();
  }
}

bool FeatureBuilder1::operator==(FeatureBuilder1 const & fb) const
{
  if (!(m_params == fb.m_params))
    return false;

  if (m_coastCell != fb.m_coastCell)
    return false;

  if (m_params.GetGeomType() == GEOM_POINT && !IsEqual(m_center, fb.m_center))
    return false;

  if (!IsEqual(m_limitRect, fb.m_limitRect))
    return false;

  if (m_polygons.size() != fb.m_polygons.size())
    return false;

  if (m_osmIds != fb.m_osmIds)
    return false;

  for (auto i = m_polygons.cbegin(), j = fb.m_polygons.cbegin(); i != m_polygons.cend(); ++i, ++j)
  {
    if (!IsEqual(*i, *j))
      return false;
  }

  return true;
}

bool FeatureBuilder1::CheckValid() const
{
  CHECK(m_params.CheckValid(), (*this));

  EGeomType const type = m_params.GetGeomType();

  if (type == GEOM_LINE)
    CHECK(GetOuterGeometry().size() >= 2, (*this));

  if (type == GEOM_AREA)
    for (PointSeq const & points : m_polygons)
      CHECK(points.size() >= 3, (*this));

  return true;
}

void FeatureBuilder1::SerializeBase(Buffer & data, serial::GeometryCodingParams const & params,
                                    bool saveAddInfo) const
{
  PushBackByteSink<Buffer> sink(data);

  m_params.Write(sink, saveAddInfo);

  if (m_params.GetGeomType() == GEOM_POINT)
    serial::SavePoint(sink, m_center, params);
}

void FeatureBuilder1::Serialize(Buffer & data) const
{
  CHECK ( CheckValid(), (*this) );

  data.clear();

  serial::GeometryCodingParams cp;

  SerializeBase(data, cp, true /* store additional info from FeatureParams */);

  PushBackByteSink<Buffer> sink(data);

  if (m_params.GetGeomType() != GEOM_POINT)
  {
    WriteVarUint(sink, static_cast<uint32_t>(m_polygons.size()));

    for (PointSeq const & points : m_polygons)
      serial::SaveOuterPath(points, cp, sink);

    WriteVarInt(sink, m_coastCell);
  }

  // save OSM IDs to link meta information with sorted features later
  rw::WriteVectorOfPOD(sink, m_osmIds);

  // check for correct serialization
#ifdef DEBUG
  Buffer tmp(data);
  FeatureBuilder1 fb;
  fb.Deserialize(tmp);
  ASSERT ( fb == *this, ("Source feature: ", *this, "Deserialized feature: ", fb) );
#endif
}

void FeatureBuilder1::SerializeBorder(serial::GeometryCodingParams const & params,
                                      Buffer & data) const
{
  data.clear();

  PushBackByteSink<Buffer> sink(data);
  WriteToSink(sink, GetMostGenericOsmId().GetEncodedId());

  CHECK_GREATER(m_polygons.size(), 0, ());

  WriteToSink(sink, m_polygons.size() - 1);

  auto toU = [&params](m2::PointD const & p) { return PointDToPointU(p, params.GetCoordBits()); };
  for (auto const & polygon : m_polygons)
  {
    WriteToSink(sink, polygon.size());
    m2::PointU last = params.GetBasePoint();
    for (auto const & p : polygon)
    {
      auto const curr = toU(p);
      coding::EncodePointDelta(sink, last, curr);
      last = curr;
    }
  }
}

void FeatureBuilder1::Deserialize(Buffer & data)
{
  serial::GeometryCodingParams cp;

  ArrayByteSource source(&data[0]);
  m_params.Read(source);

  m_limitRect.MakeEmpty();

  EGeomType const type = m_params.GetGeomType();
  if (type == GEOM_POINT)
  {
    m_center = serial::LoadPoint(source, cp);
    m_limitRect.Add(m_center);
  }
  else
  {
    m_polygons.clear();
    uint32_t const count = ReadVarUint<uint32_t>(source);
    ASSERT_GREATER ( count, 0, (*this) );

    for (uint32_t i = 0; i < count; ++i)
    {
      m_polygons.push_back(PointSeq());
      serial::LoadOuterPath(source, cp, m_polygons.back());
      CalcRect(m_polygons.back(), m_limitRect);
    }

    m_coastCell = ReadVarInt<int64_t>(source);
  }

  rw::ReadVectorOfPOD(source, m_osmIds);

  CHECK ( CheckValid(), (*this) );
}

void FeatureBuilder1::AddOsmId(base::GeoObjectId id) { m_osmIds.push_back(id); }

void FeatureBuilder1::SetOsmId(base::GeoObjectId id) { m_osmIds.assign(1, id); }

base::GeoObjectId FeatureBuilder1::GetFirstOsmId() const
{
  ASSERT(!m_osmIds.empty(), ());
  return m_osmIds.front();
}

base::GeoObjectId FeatureBuilder1::GetLastOsmId() const
{
  ASSERT(!m_osmIds.empty(), ());
  return m_osmIds.back();
}

base::GeoObjectId FeatureBuilder1::GetMostGenericOsmId() const
{
  ASSERT(!m_osmIds.empty(), ());
  auto result = m_osmIds.front();
  for (auto const & id : m_osmIds)
  {
    auto const t = id.GetType();
    if (t == base::GeoObjectId::Type::ObsoleteOsmRelation)
    {
      result = id;
      break;
    }
    else if (t == base::GeoObjectId::Type::ObsoleteOsmWay &&
             result.GetType() == base::GeoObjectId::Type::ObsoleteOsmNode)
    {
      result = id;
    }
  }
  return result;
}

bool FeatureBuilder1::HasOsmId(base::GeoObjectId const & id) const
{
  for (auto const & cid : m_osmIds)
  {
    if (cid == id)
      return true;
  }
  return false;
}

int FeatureBuilder1::GetMinFeatureDrawScale() const
{
  int const minScale = feature::GetMinDrawableScale(GetTypesHolder(), m_limitRect);

  // some features become invisible after merge processing, so -1 is possible
  return (minScale == -1 ? 1000 : minScale);
}

bool FeatureBuilder1::AddName(string const & lang, string const & name)
{
  return m_params.AddName(lang, name);
}

string FeatureBuilder1::GetName(int8_t lang) const
{
  string s;
  VERIFY(m_params.name.GetString(lang, s) != s.empty(), ());
  return s;
}

size_t FeatureBuilder1::GetPointsCount() const
{
  size_t counter = 0;
  for (auto const & p : m_polygons)
    counter += p.size();
  return counter;
}

string DebugPrint(FeatureBuilder1 const & f)
{
  ostringstream out;

  switch (f.GetGeomType())
  {
  case GEOM_POINT: out << DebugPrint(f.m_center); break;
  case GEOM_LINE: out << "line with " << f.GetPointsCount() << " points"; break;
  case GEOM_AREA: out << "area with " << f.GetPointsCount() << " points"; break;
  default: out << "ERROR: unknown geometry type"; break;
  }

  out << " " << DebugPrint(f.m_limitRect) << " " << DebugPrint(f.m_params) << " " << DebugPrint(f.m_osmIds);
  return out.str();
}

bool FeatureBuilder1::IsDrawableInRange(int lowScale, int highScale) const
{
  if (!GetOuterGeometry().empty())
  {
    auto const types = GetTypesHolder();
    while (lowScale <= highScale)
    {
      if (feature::IsDrawableForIndex(types, m_limitRect, lowScale++))
        return true;
    }
  }

  return false;
}

uint64_t FeatureBuilder1::GetWayIDForRouting() const
{
  if (m_osmIds.size() == 1 && m_osmIds[0].GetType() == base::GeoObjectId::Type::ObsoleteOsmWay &&
      IsLine() && IsRoad())
    return m_osmIds[0].GetSerialId();
  return 0;
}

string DebugPrint(FeatureBuilder2 const & f)
{
  return DebugPrint(static_cast<FeatureBuilder1 const &>(f));
}

bool FeatureBuilder2::PreSerializeAndRemoveUselessNames(SupportingData const & data)
{
  // make flags actual before header serialization
  EGeomType const geoType = m_params.GetGeomType();
  if (geoType == GEOM_LINE)
  {
    if (data.m_ptsMask == 0 && data.m_innerPts.empty())
      return false;
  }
  else if (geoType == GEOM_AREA)
  {
    if (data.m_trgMask == 0 && data.m_innerTrg.empty())
      return false;
  }

  // we don't need empty features without geometry
  return Base::PreSerializeAndRemoveUselessNames();
}

void FeatureBuilder2::SerializeLocalityObject(serial::GeometryCodingParams const & params,
                                              SupportingData & data) const
{
  data.m_buffer.clear();

  PushBackByteSink<Buffer> sink(data.m_buffer);
  WriteToSink(sink, GetMostGenericOsmId().GetEncodedId());

  auto const type = m_params.GetGeomType();
  WriteToSink(sink, static_cast<uint8_t>(type));

  if (type == GEOM_POINT)
  {
    serial::SavePoint(sink, m_center, params);
    return;
  }

  CHECK_EQUAL(type, GEOM_AREA, ("Supported types are GEOM_POINT and GEOM_AREA"));

  uint32_t trgCount = base::asserted_cast<uint32_t>(data.m_innerTrg.size());
  CHECK_GREATER(trgCount, 2, ());
  trgCount -= 2;

  WriteToSink(sink, trgCount);
  serial::SaveInnerTriangles(data.m_innerTrg, params, sink);
}

void FeatureBuilder2::Serialize(SupportingData & data,
                                serial::GeometryCodingParams const & params) const
{
  data.m_buffer.clear();

  // header data serialization
  SerializeBase(data.m_buffer, params, false /* don't store additional info from FeatureParams*/);

  PushBackByteSink<Buffer> sink(data.m_buffer);

  uint8_t const ptsCount = base::asserted_cast<uint8_t>(data.m_innerPts.size());
  uint8_t trgCount = base::asserted_cast<uint8_t>(data.m_innerTrg.size());
  if (trgCount > 0)
  {
    ASSERT_GREATER ( trgCount, 2, () );
    trgCount -= 2;
  }

  EGeomType const type = m_params.GetGeomType();

  {
    BitWriter<PushBackByteSink<Buffer>> bitSink(sink);

    if (type == GEOM_LINE)
    {
      bitSink.Write(ptsCount, 4);
      if (ptsCount == 0)
        bitSink.Write(data.m_ptsMask, 4);
    }
    else if (type == GEOM_AREA)
    {
      bitSink.Write(trgCount, 4);
      if (trgCount == 0)
        bitSink.Write(data.m_trgMask, 4);
    }
  }

  if (type == GEOM_LINE)
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
      ASSERT_GREATER ( GetOuterGeometry().size(), 2, () );

      // Store first point once for outer linear features.
      serial::SavePoint(sink, GetOuterGeometry()[0], params);

      // offsets was pushed from high scale index to low
      reverse(data.m_ptsOffset.begin(), data.m_ptsOffset.end());
      WriteVarUintArray(data.m_ptsOffset, sink);
    }
  }
  else if (type == GEOM_AREA)
  {
    if (trgCount > 0)
      serial::SaveInnerTriangles(data.m_innerTrg, params, sink);
    else
    {
      // offsets was pushed from high scale index to low
      reverse(data.m_trgOffset.begin(), data.m_trgOffset.end());
      WriteVarUintArray(data.m_trgOffset, sink);
    }
  }
}
