#include "generator/feature_builder.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/bit_streams.hpp"
#include "coding/byte_stream.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"

#include "geometry/region2d.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

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

bool IsEqual(std::vector<m2::PointD> const & v1, std::vector<m2::PointD> const & v2)
{
  return equal(cbegin(v1), cend(v1), cbegin(v2), cend(v2),
               [](m2::PointD const & p1, m2::PointD const & p2) { return IsEqual(p1, p2); });
}

template <class Sink, class T>
void WritePOD(Sink & sink, T const & value)
{
  static_assert(std::is_trivially_copyable<T>::value, "");

  sink.Write(&value, sizeof(T));
}

template <class Sink, class T>
void ReadPOD(Sink & src, T & value)
{
  static_assert(std::is_trivially_copyable<T>::value, "");

  src.Read(&value, sizeof(T));
}
}  // namespace

namespace feature
{
FeatureBuilder::FeatureBuilder()
  : m_coastCell(-1)
{
  m_polygons.push_back(PointSeq());
}

bool FeatureBuilder::IsGeometryClosed() const
{
  PointSeq const & poly = GetOuterGeometry();
  return (poly.size() > 2 && poly.front() == poly.back());
}

m2::PointD FeatureBuilder::GetGeometryCenter() const
{
  m2::PointD ret(0.0, 0.0);

  PointSeq const & poly = GetOuterGeometry();
  size_t const count = poly.size();
  for (size_t i = 0; i < count; ++i)
    ret += poly[i];
  return ret / count;
}

m2::PointD FeatureBuilder::GetKeyPoint() const
{
  switch (GetGeomType())
  {
  case GeomType::Point:
    return m_center;
  case GeomType::Line:
  case GeomType::Area:
    return GetGeometryCenter();
  default:
    CHECK(false, ());
    return m2::PointD();
  }
}

void FeatureBuilder::SetCenter(m2::PointD const & p)
{
  ResetGeometry();
  m_center = p;
  m_params.SetGeomType(GeomType::Point);
  m_limitRect.Add(p);
}

void FeatureBuilder::SetRank(uint8_t rank)
{
  m_params.rank = rank;
}

void FeatureBuilder::AddPoint(m2::PointD const & p)
{
  m_polygons.front().push_back(p);
  m_limitRect.Add(p);
}

void FeatureBuilder::SetLinear(bool reverseGeometry)
{
  m_params.SetGeomType(GeomType::Line);
  m_polygons.resize(1);

  if (reverseGeometry)
  {
    auto & cont = m_polygons.front();
    ASSERT(!cont.empty(), ());
    reverse(cont.begin(), cont.end());
  }
}

void FeatureBuilder::SetHoles(FeatureBuilder::Geometry const & holes)
{
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

void FeatureBuilder::AddPolygon(std::vector<m2::PointD> & poly)
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

void FeatureBuilder::ResetGeometry()
{
  m_polygons.clear();
  m_polygons.push_back(PointSeq());
  m_limitRect.MakeEmpty();
}

bool FeatureBuilder::RemoveInvalidTypes()
{
  if (!m_params.FinishAddingTypes())
    return false;

  return RemoveUselessTypes(m_params.m_types, m_params.GetGeomType(), m_params.IsEmptyNames());
}

TypesHolder FeatureBuilder::GetTypesHolder() const
{
  CHECK(IsValid(), (*this));

  TypesHolder holder(m_params.GetGeomType());
  for (auto const t : m_params.m_types)
    holder.Add(t);

  return holder;
}

bool FeatureBuilder::PreSerialize()
{
  if (!m_params.IsValid())
    return false;

  switch (m_params.GetGeomType())
  {
  case GeomType::Point:
    // Store house number like HEADER_GEOM_POINT_EX.
    if (!m_params.house.IsEmpty())
    {
      m_params.SetGeomTypePointEx();
      m_params.rank = 0;
    }

    if (!m_params.ref.empty())
    {

      if (ftypes::IsMotorwayJunctionChecker::Instance()(GetTypes()) ||
          (m_params.name.IsEmpty() &&
           (ftypes::IsPostBoxChecker::Instance()(GetTypes()) ||
            ftypes::IsRailwaySubwayEntranceChecker::Instance()(GetTypes()) ||
            ftypes::IsEntranceChecker::Instance()(GetTypes()) ||
            ftypes::IsRailwayStationChecker::Instance()(GetTypes()))))
      {
        m_params.name.AddString(StringUtf8Multilang::kDefaultCode, m_params.ref);
      }
    }

    m_params.ref.clear();
    break;

  case GeomType::Line:
  {
    // We need refs for road's numbers.
    if (!routing::IsRoad(GetTypes()))
      m_params.ref.clear();

    m_params.rank = 0;
    m_params.house.Clear();
    break;
  }

  case GeomType::Area:
    m_params.rank = 0;
    m_params.ref.clear();
    break;

  default:
    return false;
  }

  return true;
}

bool FeatureBuilder::PreSerializeAndRemoveUselessNamesForIntermediate()
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

void FeatureBuilder::RemoveUselessNames()
{
  if (!m_params.name.IsEmpty() && !IsCoastCell())
  {
    // Remove names for boundary-administrative-* features.
    // AFAIR, they were very messy in search because they contain places' names.
    auto const typeRemover = [](uint32_t type)
    {
      static TypeSetChecker const checkBoundary({ "boundary", "administrative" });
      return checkBoundary.IsEqual(type);
    };

    auto types = GetTypesHolder();
    if (types.RemoveIf(typeRemover))
    {
      // Remove only if there are no other text-style types in feature (e.g. highway).
      std::pair<int, int> const range = GetDrawableScaleRangeForRules(types, RULE_ANY_TEXT);
      if (range.first == -1)
        m_params.name.Clear();
    }

    // We want to skip alt_name which is almost equal to name.
    std::string_view name, altName;
    if (m_params.name.GetString(StringUtf8Multilang::kAltNameCode, altName) &&
        m_params.name.GetString(StringUtf8Multilang::kDefaultCode, name) &&
        search::NormalizeAndSimplifyString(altName) == search::NormalizeAndSimplifyString(name))
    {
      m_params.name.RemoveString(StringUtf8Multilang::kAltNameCode);
    }
  }
}

void FeatureBuilder::RemoveNameIfInvisible(int minS, int maxS)
{
  if (!m_params.name.IsEmpty() && !IsCoastCell())
  {
    std::pair<int, int> const range = GetDrawableScaleRangeForRules(GetTypesHolder(), RULE_ANY_TEXT);
    if (range.first > maxS || range.second < minS)
      m_params.name.Clear();
  }
}

bool FeatureBuilder::operator==(FeatureBuilder const & fb) const
{
  if (!(m_params == fb.m_params))
    return false;

  if (m_coastCell != fb.m_coastCell)
    return false;

  if (m_params.GetGeomType() == GeomType::Point && !IsEqual(m_center, fb.m_center))
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

bool FeatureBuilder::IsExactEq(FeatureBuilder const & fb) const
{
  if (m_params.GetGeomType() == GeomType::Point && m_center != fb.m_center)
    return false;

  return (m_polygons == fb.m_polygons && m_limitRect == fb.m_limitRect &&
          m_osmIds == fb.m_osmIds && m_params == fb.m_params && m_coastCell == fb.m_coastCell);
}

void FeatureBuilder::SerializeForIntermediate(Buffer & data) const
{
  CHECK(IsValid(), (*this));

  data.clear();

  serial::GeometryCodingParams cp;

  PushBackByteSink<Buffer> sink(data);
  m_params.Write(sink);

  if (m_params.GetGeomType() == GeomType::Point)
  {
    serial::SavePoint(sink, m_center, cp);
  }
  else
  {
    WriteVarUint(sink, static_cast<uint32_t>(m_polygons.size()));

    for (PointSeq const & points : m_polygons)
      serial::SaveOuterPath(points, cp, sink);

    WriteVarInt(sink, m_coastCell);
  }

  // Save OSM IDs to link meta information with sorted features later.
  rw::WriteVectorOfPOD(sink, m_osmIds);

  // Check for correct serialization.
#ifdef DEBUG
  Buffer tmp(data);
  FeatureBuilder fb;
  fb.DeserializeFromIntermediate(tmp);
  ASSERT ( fb == *this, ("Source feature: ", *this, "Deserialized feature: ", fb) );
#endif
}

void FeatureBuilder::SerializeBorderForIntermediate(serial::GeometryCodingParams const & params,
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

void FeatureBuilder::DeserializeFromIntermediate(Buffer & data)
{
  serial::GeometryCodingParams cp;

  ArrayByteSource source(&data[0]);
  m_params.Read(source);

  m_limitRect.MakeEmpty();

  GeomType const type = m_params.GetGeomType();
  if (type == GeomType::Point)
  {
    m_center = serial::LoadPoint(source, cp);
    m_limitRect.Add(m_center);
  }
  else
  {
    m_polygons.clear();
    uint32_t const count = ReadVarUint<uint32_t>(source);
    ASSERT_GREATER( count, 0, (*this) );

    for (uint32_t i = 0; i < count; ++i)
    {
      m_polygons.push_back(PointSeq());
      serial::LoadOuterPath(source, cp, m_polygons.back());
      CalcRect(m_polygons.back(), m_limitRect);
    }

    m_coastCell = ReadVarInt<int64_t>(source);
  }

  rw::ReadVectorOfPOD(source, m_osmIds);

  CHECK(IsValid(), (*this));
}

void FeatureBuilder::SerializeAccuratelyForIntermediate(Buffer & data) const
{
  CHECK(IsValid(), (*this));

  data.clear();
  PushBackByteSink<Buffer> sink(data);
  m_params.Write(sink);
  if (IsPoint())
  {
    WritePOD(sink, m_center);
  }
  else
  {
    WriteVarUint(sink, static_cast<uint32_t>(m_polygons.size()));
    for (PointSeq const & points : m_polygons)
      rw::WriteVectorOfPOD(sink, points);

    WriteVarInt(sink, m_coastCell);
  }

  // Save OSM IDs to link meta information with sorted features later.
  rw::WriteVectorOfPOD(sink, m_osmIds);
  // Check for correct serialization.
#ifdef DEBUG
  Buffer tmp(data);
  FeatureBuilder fb;
  fb.DeserializeAccuratelyFromIntermediate(tmp);
  ASSERT ( fb == *this, ("Source feature: ", *this, "Deserialized feature: ", fb) );
#endif

}

void FeatureBuilder::DeserializeAccuratelyFromIntermediate(Buffer & data)
{
  ArrayByteSource source(&data[0]);
  m_params.Read(source);
  m_limitRect.MakeEmpty();
  if (IsPoint())
  {
    ReadPOD(source, m_center);
    m_limitRect.Add(m_center);
  }
  else
  {
    m_polygons.clear();
    uint32_t const count = ReadVarUint<uint32_t>(source);
    ASSERT_GREATER(count, 0, (*this));
    for (uint32_t i = 0; i < count; ++i)
    {
      m_polygons.push_back(PointSeq());
      rw::ReadVectorOfPOD(source, m_polygons.back());
      CalcRect(m_polygons.back(), m_limitRect);
    }

    m_coastCell = ReadVarInt<int64_t>(source);
  }

  rw::ReadVectorOfPOD(source, m_osmIds);

  CHECK(IsValid(), (*this));
}

void FeatureBuilder::AddOsmId(base::GeoObjectId id) { m_osmIds.push_back(id); }

void FeatureBuilder::SetOsmId(base::GeoObjectId id) { m_osmIds.assign(1, id); }

base::GeoObjectId FeatureBuilder::GetFirstOsmId() const
{
  ASSERT(!m_osmIds.empty(), ());
  return m_osmIds.front();
}

base::GeoObjectId FeatureBuilder::GetLastOsmId() const
{
  ASSERT(!m_osmIds.empty(), ());
  return m_osmIds.back();
}

base::GeoObjectId FeatureBuilder::GetMostGenericOsmId() const
{
  if (m_osmIds.empty())
    return base::GeoObjectId();

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

bool FeatureBuilder::HasOsmId(base::GeoObjectId const & id) const
{
  for (auto const & cid : m_osmIds)
  {
    if (cid == id)
      return true;
  }
  return false;
}

int FeatureBuilder::GetMinFeatureDrawScale() const
{
  int const minScale = GetMinDrawableScale(GetTypesHolder(), m_limitRect);

  // some features become invisible after merge processing, so -1 is possible
  return (minScale == -1 ? 1000 : minScale);
}

bool FeatureBuilder::AddName(std::string_view lang, std::string_view name)
{
  return m_params.AddName(lang, name);
}

std::string_view FeatureBuilder::GetName(int8_t lang) const
{
  std::string_view sv;
  CHECK(m_params.name.GetString(lang, sv) != sv.empty(), ());
  return sv;
}

size_t FeatureBuilder::GetPointsCount() const
{
  size_t counter = 0;
  for (auto const & p : m_polygons)
    counter += p.size();
  return counter;
}

bool FeatureBuilder::IsDrawableInRange(int lowScale, int highScale) const
{
  if (!GetOuterGeometry().empty())
  {
    auto const types = GetTypesHolder();
    while (lowScale <= highScale)
    {
      if (IsDrawableForIndex(types, m_limitRect, lowScale++))
        return true;
    }
  }

  return false;
}

bool FeatureBuilder::PreSerializeAndRemoveUselessNamesForMwm(SupportingData const & data)
{
  // We don't need empty features without geometry.
  GeomType const geomType = m_params.GetGeomType();
  if (geomType == GeomType::Line)
  {
    if (data.m_ptsMask == 0 && data.m_innerPts.empty())
    {
      LOG(LWARNING, ("Skip feature with empty geometry", GetMostGenericOsmId()));
      return false;
    }
  }
  else if (geomType == GeomType::Area)
  {
    if (data.m_trgMask == 0 && data.m_innerTrg.empty())
    {
      LOG(LWARNING, ("Skip feature with empty geometry", GetMostGenericOsmId()));
      return false;
    }
  }

  return PreSerializeAndRemoveUselessNamesForIntermediate();
}

void FeatureBuilder::SerializeLocalityObject(serial::GeometryCodingParams const & params,
                                             SupportingData & data) const
{
  data.m_buffer.clear();

  PushBackByteSink<Buffer> sink(data.m_buffer);
  WriteToSink(sink, GetMostGenericOsmId().GetEncodedId());

  auto const type = m_params.GetGeomType();
  WriteToSink(sink, static_cast<uint8_t>(type));

  if (type == GeomType::Point)
  {
    serial::SavePoint(sink, m_center, params);
    return;
  }

  CHECK_EQUAL(type, GeomType::Area, ("Supported types are Point and Area"));

  uint32_t trgCount = base::asserted_cast<uint32_t>(data.m_innerTrg.size());
  CHECK_GREATER(trgCount, 2, ());
  trgCount -= 2;

  WriteToSink(sink, trgCount);
  serial::SaveInnerTriangles(data.m_innerTrg, params, sink);
}

void FeatureBuilder::SerializeForMwm(SupportingData & data,
                                     serial::GeometryCodingParams const & params) const
{
  data.m_buffer.clear();

  PushBackByteSink<Buffer> sink(data.m_buffer);
  FeatureParams(m_params).Write(sink);

  if (m_params.GetGeomType() == GeomType::Point)
  {
    serial::SavePoint(sink, m_center, params);
    return;
  }

  uint8_t const ptsCount = base::asserted_cast<uint8_t>(data.m_innerPts.size());
  uint8_t trgCount = base::asserted_cast<uint8_t>(data.m_innerTrg.size());
  if (trgCount > 0)
  {
    ASSERT_GREATER ( trgCount, 2, () );
    trgCount -= 2;
  }

  GeomType const type = m_params.GetGeomType();

  {
    BitWriter<PushBackByteSink<Buffer>> bitSink(sink);

    if (type == GeomType::Line)
    {
      bitSink.Write(ptsCount, 4);
      if (ptsCount == 0)
        bitSink.Write(data.m_ptsMask, 4);
    }
    else if (type == GeomType::Area)
    {
      bitSink.Write(trgCount, 4);
      if (trgCount == 0)
        bitSink.Write(data.m_trgMask, 4);
    }
  }

  if (type == GeomType::Line)
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
  else if (type == GeomType::Area)
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

bool FeatureBuilder::IsValid() const
{
  if (!GetParams().IsValid())
    return false;

  if (IsLine() && GetOuterGeometry().size() < 2)
    return false;

  if (IsArea())
  {
    for (auto const & points : GetGeometry())
    {
      if (points.size() < 3)
        return false;
    }
  }

  return true;
}

std::string DebugPrint(FeatureBuilder const & fb)
{
  std::ostringstream out;

  switch (fb.GetGeomType())
  {
  case GeomType::Point: out << DebugPrint(fb.GetKeyPoint()); break;
  case GeomType::Line: out << "line with " << fb.GetPointsCount() << " points"; break;
  case GeomType::Area: out << "area with " << fb.GetPointsCount() << " points"; break;
  default: out << "ERROR: unknown geometry type"; break;
  }

  out << " " << DebugPrint(fb.GetLimitRect())
      << " " << DebugPrint(fb.GetParams())
      << " " << ::DebugPrint(fb.m_osmIds);
  return out.str();
}

namespace serialization_policy
{
// static
TypeSerializationVersion const MinSize::kSerializationVersion;
// static
TypeSerializationVersion const MaxAccuracy::kSerializationVersion;
}  // namespace serialization_policy
}  // namespace feature
