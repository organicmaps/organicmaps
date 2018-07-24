#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_loader_base.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/distance.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/range_iterator.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>

using namespace feature;
using namespace std;

feature::EGeomType FeatureType::GetFeatureType() const
{
  switch (m_header & HEADER_GEOTYPE_MASK)
  {
  case HEADER_GEOM_LINE: return GEOM_LINE;
  case HEADER_GEOM_AREA: return GEOM_AREA;
  default: return GEOM_POINT;
  }
}

void FeatureType::SetTypes(uint32_t const (&types)[feature::kMaxTypesCount], uint32_t count)
{
  ASSERT_GREATER_OR_EQUAL(count, 1, ("Must be one type at least."));
  ASSERT_LESS(count, feature::kMaxTypesCount, ("Too many types for feature"));
  if (count >= feature::kMaxTypesCount)
    count = feature::kMaxTypesCount - 1;
  fill(begin(m_types), end(m_types), 0);
  copy_n(begin(types), count, begin(m_types));
  auto value = static_cast<uint8_t>((count - 1) & feature::HEADER_TYPE_MASK);
  m_header = (m_header & (~feature::HEADER_TYPE_MASK)) | value;
}

void FeatureType::Deserialize(feature::LoaderBase * loader, Buffer buffer)
{
  m_loader = loader;
  m_data = buffer;
  m_offsets.m_common = m_offsets.m_header2 = 0;
  m_ptsSimpMask = 0;
  m_ptsOffsets.clear();
  m_trgOffsets.clear();

  m_limitRect = m2::RectD::GetEmptyRect();
  m_header = m_loader->GetHeader(*this);

  m_parsed = {};

  m_innerStats.MakeZero();
}

void FeatureType::ParseTypes()
{
  if (!m_parsed.m_types)
  {
    m_loader->ParseTypes(*this);
    m_parsed.m_types = true;
  }
}

void FeatureType::ParseCommon()
{
  if (!m_parsed.m_common)
  {
    ParseTypes();

    m_loader->ParseCommon(*this);
    m_parsed.m_common = true;
  }
}

void FeatureType::ReplaceBy(osm::EditableMapObject const & emo)
{
  uint8_t geoType;
  if (emo.IsPointType())
  {
    // We are here for existing point features and for newly created point features.
    m_center = emo.GetMercator();
    m_limitRect.MakeEmpty();
    m_limitRect.Add(m_center);
    m_parsed.m_points = m_parsed.m_triangles = true;
    geoType = feature::GEOM_POINT;
  }
  else
  {
    geoType = m_header & HEADER_GEOTYPE_MASK;
  }

  m_params.name = emo.GetName();
  string const & house = emo.GetHouseNumber();
  if (house.empty())
    m_params.house.Clear();
  else
    m_params.house.Set(house);
  m_parsed.m_common = true;

  m_metadata = emo.GetMetadata();
  m_parsed.m_metadata = true;

  uint32_t typesCount = 0;
  for (uint32_t const type : emo.GetTypes())
    m_types[typesCount++] = type;
  m_parsed.m_types = true;
  m_header = CalculateHeader(typesCount, geoType, m_params);
  m_parsed.m_header2 = true;

  m_id = emo.GetID();
}

void FeatureType::ParseEverything()
{
  // Also calls ParseCommon() and ParseTypes().
  ParseHeader2();
  ParseGeometry(FeatureType::BEST_GEOMETRY);
  ParseTriangles(FeatureType::BEST_GEOMETRY);
  ParseMetadata();
}

void FeatureType::ParseHeader2()
{
  if (!m_parsed.m_header2)
  {
    ParseCommon();

    m_loader->ParseHeader2(*this);
    m_parsed.m_header2 = true;
  }
}

void FeatureType::ResetGeometry()
{
  m_points.clear();
  m_triangles.clear();

  if (GetFeatureType() != GEOM_POINT)
    m_limitRect = m2::RectD();

  m_parsed.m_header2 = m_parsed.m_points = m_parsed.m_triangles = false;
  m_ptsSimpMask = 0;

  m_ptsOffsets.clear();
  m_trgOffsets.clear();
}

uint32_t FeatureType::ParseGeometry(int scale)
{
  uint32_t sz = 0;
  if (!m_parsed.m_points)
  {
    ParseHeader2();

    sz = m_loader->ParseGeometry(scale, *this);
    m_parsed.m_points = true;
  }
  return sz;
}

uint32_t FeatureType::ParseTriangles(int scale)
{
  uint32_t sz = 0;
  if (!m_parsed.m_triangles)
  {
    ParseHeader2();

    sz = m_loader->ParseTriangles(scale, *this);
    m_parsed.m_triangles = true;
  }
  return sz;
}

void FeatureType::ParseMetadata()
{
  if (m_parsed.m_metadata)
    return;

  m_loader->ParseMetadata(*this);

  m_parsed.m_metadata = true;
}

StringUtf8Multilang const & FeatureType::GetNames()
{
  ParseCommon();
  return m_params.name;
}

void FeatureType::SetNames(StringUtf8Multilang const & newNames)
{
  m_params.name.Clear();
  // Validate passed string to clean up empty names (if any).
  newNames.ForEach([this](int8_t langCode, string const & name) {
    if (!name.empty())
      m_params.name.AddString(langCode, name);
  });

  if (m_params.name.IsEmpty())
    m_header = ~feature::HEADER_HAS_NAME & m_header;
  else
    m_header = feature::HEADER_HAS_NAME | m_header;
}

void FeatureType::SetMetadata(feature::Metadata const & newMetadata)
{
  m_parsed.m_metadata = true;
  m_metadata = newMetadata;
}

void FeatureType::UpdateHeader(bool commonParsed, bool metadataParsed)
{
  feature::EHeaderTypeMask geomType =
      static_cast<feature::EHeaderTypeMask>(m_header & feature::HEADER_GEOTYPE_MASK);
  if (!geomType)
  {
    geomType = m_params.house.IsEmpty() && !m_params.ref.empty() ? feature::HEADER_GEOM_POINT
                                                                 : feature::HEADER_GEOM_POINT_EX;
  }

  m_header = feature::CalculateHeader(GetTypesCount(), geomType, m_params);
  m_parsed.m_header2 = true;
  m_parsed.m_types = true;

  m_parsed.m_common = commonParsed;
  m_parsed.m_metadata = metadataParsed;
}

bool FeatureType::UpdateMetadataValue(string const & key, string const & value)
{
  feature::Metadata::EType mdType;
  if (!feature::Metadata::TypeFromString(key, mdType))
    return false;
  m_metadata.Set(mdType, value);
  return true;
}

void FeatureType::ForEachMetadataItem(
    bool skipSponsored, function<void(string const & tag, string const & value)> const & fn) const
{
  for (auto const type : m_metadata.GetPresentTypes())
  {
    if (skipSponsored && m_metadata.IsSponsoredType(static_cast<feature::Metadata::EType>(type)))
      continue;
    auto const attributeName = ToString(static_cast<feature::Metadata::EType>(type));
    fn(attributeName, m_metadata.Get(type));
  }
}

void FeatureType::SetCenter(m2::PointD const & pt)
{
  ASSERT_EQUAL(GetFeatureType(), GEOM_POINT, ("Only for point feature."));
  m_center = pt;
  m_limitRect.Add(m_center);
  m_parsed.m_points = m_parsed.m_triangles = true;
}

namespace
{
  template <class TCont>
  void Points2String(string & s, TCont const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
      s += DebugPrint(points[i]) + " ";
  }
}

string FeatureType::DebugString(int scale)
{
  ParseCommon();

  Classificator const & c = classif();

  string res = "Types";
  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += (" : " + c.GetReadableObjectName(m_types[i]));
  res += "\n";

  res += m_params.DebugString();

  ParseGeometryAndTriangles(scale);
  switch (GetFeatureType())
  {
  case GEOM_POINT: res += (" Center:" + DebugPrint(m_center)); break;

  case GEOM_LINE:
    res += " Points:";
    Points2String(res, m_points);
    break;

  case GEOM_AREA:
    res += " Triangles:";
    Points2String(res, m_triangles);
    break;

  case GEOM_UNDEFINED:
    ASSERT(false, ("Assume that we have valid feature always"));
    break;
  }

  return res;
}

bool FeatureType::IsEmptyGeometry(int scale)
{
  ParseGeometryAndTriangles(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_triangles.empty();
  case GEOM_LINE: return m_points.empty();
  default: return false;
  }
}

m2::RectD FeatureType::GetLimitRect(int scale)
{
  ParseGeometryAndTriangles(scale);

  if (m_triangles.empty() && m_points.empty() && (GetFeatureType() != GEOM_POINT))
  {
    // This function is called during indexing, when we need
    // to check visibility according to feature sizes.
    // So, if no geometry for this scale, assume that rect has zero dimensions.
    m_limitRect = m2::RectD(0, 0, 0, 0);
  }

  return m_limitRect;
}

void FeatureType::ParseGeometryAndTriangles(int scale)
{
  ParseGeometry(scale);
  ParseTriangles(scale);
}

FeatureType::GeomStat FeatureType::GetGeometrySize(int scale)
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_points.empty())
    sz = m_innerStats.m_points;

  return GeomStat(sz, m_points.size());
}

FeatureType::GeomStat FeatureType::GetTrianglesSize(int scale)
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_triangles.empty())
    sz = m_innerStats.m_strips;

  return GeomStat(sz, m_triangles.size());
}

void FeatureType::GetPreferredNames(string & primary, string & secondary)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  ::GetPreferredNames(mwmInfo->GetRegionData(), GetNames(), deviceLang, false /* allowTranslit */,
                      primary, secondary);
}

void FeatureType::GetPreferredNames(bool allowTranslit, int8_t deviceLang, string & primary,
                                    string & secondary)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  ::GetPreferredNames(mwmInfo->GetRegionData(), GetNames(), deviceLang, allowTranslit,
                      primary, secondary);
}

void FeatureType::GetReadableName(string & name)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  ::GetReadableName(mwmInfo->GetRegionData(), GetNames(), deviceLang, false /* allowTranslit */,
                    name);
}

void FeatureType::GetReadableName(bool allowTranslit, int8_t deviceLang, string & name)
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  ::GetReadableName(mwmInfo->GetRegionData(), GetNames(), deviceLang, allowTranslit, name);
}

string FeatureType::GetHouseNumber()
{
  ParseCommon();
  return m_params.house.Get();
}

void FeatureType::SetHouseNumber(string const & number)
{
  if (number.empty())
    m_params.house.Clear();
  else
    m_params.house.Set(number);
}

bool FeatureType::GetName(int8_t lang, string & name)
{
  if (!HasName())
    return false;

  ParseCommon();
  return m_params.name.GetString(lang, name);
}

uint8_t FeatureType::GetRank()
{
  ParseCommon();
  return m_params.rank;
}

uint64_t FeatureType::GetPopulation() { return feature::RankToPopulation(GetRank()); }

string FeatureType::GetRoadNumber()
{
  ParseCommon();
  return m_params.ref;
}
