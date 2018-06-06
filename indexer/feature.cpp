#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_loader_base.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"

#include "editor/osm_editor.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/distance.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/range_iterator.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>

using namespace feature;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureBase implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureBase::Deserialize(feature::LoaderBase * pLoader, TBuffer buffer)
{
  m_pLoader = pLoader;
  m_pLoader->Init(buffer);

  m_limitRect = m2::RectD::GetEmptyRect();
  m_typesParsed = m_commonParsed = false;
  m_header = m_pLoader->GetHeader();
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
    m_pointsParsed = m_trianglesParsed = true;
    geoType = feature::GEOM_POINT;
  }
  else
  {
    geoType = Header() & HEADER_GEOTYPE_MASK;
  }

  m_params.name = emo.GetName();
  string const & house = emo.GetHouseNumber();
  if (house.empty())
    m_params.house.Clear();
  else
    m_params.house.Set(house);
  m_commonParsed = true;

  m_metadata = emo.GetMetadata();
  m_metadataParsed = true;

  uint32_t typesCount = 0;
  for (uint32_t const type : emo.GetTypes())
    m_types[typesCount++] = type;
  m_typesParsed = true;
  m_header = CalculateHeader(typesCount, geoType, m_params);
  m_header2Parsed = true;

  m_id = emo.GetID();
}

void FeatureBase::ParseTypes() const
{
  if (!m_typesParsed)
  {
    m_pLoader->ParseTypes();
    m_typesParsed = true;
  }
}

void FeatureBase::ParseCommon() const
{
  if (!m_commonParsed)
  {
    ParseTypes();

    m_pLoader->ParseCommon();
    m_commonParsed = true;
  }
}

feature::EGeomType FeatureBase::GetFeatureType() const
{
  switch (Header() & HEADER_GEOTYPE_MASK)
  {
  case HEADER_GEOM_LINE: return GEOM_LINE;
  case HEADER_GEOM_AREA: return GEOM_AREA;
  default: return GEOM_POINT;
  }
}

void FeatureBase::SetTypes(uint32_t const (&types)[feature::kMaxTypesCount], uint32_t count)
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

string FeatureBase::DebugString() const
{
  ParseCommon();

  Classificator const & c = classif();

  string res = "Types";
  for (size_t i = 0; i < GetTypesCount(); ++i)
    res += (" : " + c.GetReadableObjectName(m_types[i]));
  res += "\n";

  return (res + m_params.DebugString());
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureType implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureType::Deserialize(feature::LoaderBase * pLoader, TBuffer buffer)
{
  base_type::Deserialize(pLoader, buffer);

  m_pLoader->InitFeature(this);

  m_header2Parsed = m_pointsParsed = m_trianglesParsed = m_metadataParsed = false;

  m_innerStats.MakeZero();
}

void FeatureType::ParseEverything() const
{
  // Also calls ParseCommon() and ParseTypes().
  ParseHeader2();
  ParseGeometry(FeatureType::BEST_GEOMETRY);
  ParseTriangles(FeatureType::BEST_GEOMETRY);
  ParseMetadata();
}

void FeatureType::ParseHeader2() const
{
  if (!m_header2Parsed)
  {
    ParseCommon();

    m_pLoader->ParseHeader2();
    m_header2Parsed = true;
  }
}

void FeatureType::ResetGeometry() const
{
  m_points.clear();
  m_triangles.clear();

  if (GetFeatureType() != GEOM_POINT)
    m_limitRect = m2::RectD();

  m_header2Parsed = m_pointsParsed = m_trianglesParsed = false;

  m_pLoader->ResetGeometry();
}

uint32_t FeatureType::ParseGeometry(int scale) const
{
  uint32_t sz = 0;
  if (!m_pointsParsed)
  {
    ParseHeader2();

    sz = m_pLoader->ParseGeometry(scale);
    m_pointsParsed = true;
  }
  return sz;
}

uint32_t FeatureType::ParseTriangles(int scale) const
{
  uint32_t sz = 0;
  if (!m_trianglesParsed)
  {
    ParseHeader2();

    sz = m_pLoader->ParseTriangles(scale);
    m_trianglesParsed = true;
  }
  return sz;
}

void FeatureType::ParseMetadata() const
{
  if (m_metadataParsed) return;

  m_pLoader->ParseMetadata();

  m_metadataParsed = true;
}

StringUtf8Multilang const & FeatureType::GetNames() const
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
    SetHeader(~feature::HEADER_HAS_NAME & Header());
  else
    SetHeader(feature::HEADER_HAS_NAME | Header());
}

void FeatureType::SetMetadata(feature::Metadata const & newMetadata)
{
  m_metadataParsed = true;
  m_metadata = newMetadata;
}

void FeatureType::UpdateHeader(bool commonParsed, bool metadataParsed)
{
  feature::EHeaderTypeMask geomType =
      static_cast<feature::EHeaderTypeMask>(Header() & feature::HEADER_GEOTYPE_MASK);
  if (!geomType)
  {
    geomType = m_params.house.IsEmpty() && !m_params.ref.empty() ? feature::HEADER_GEOM_POINT
                                                                 : feature::HEADER_GEOM_POINT_EX;
  }

  m_header = feature::CalculateHeader(GetTypesCount(), geomType, m_params);
  m_header2Parsed = true;
  m_typesParsed = true;

  m_commonParsed = commonParsed;
  m_metadataParsed = metadataParsed;
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
  m_pointsParsed = m_trianglesParsed = true;
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

string FeatureType::DebugString(int scale) const
{
  ParseGeometryAndTriangles(scale);

  string s = base_type::DebugString();

  switch (GetFeatureType())
  {
  case GEOM_POINT:
    s += (" Center:" + DebugPrint(m_center));
    break;

  case GEOM_LINE:
    s += " Points:";
    Points2String(s, m_points);
    break;

  case GEOM_AREA:
    s += " Triangles:";
    Points2String(s, m_triangles);
    break;

  case GEOM_UNDEFINED:
    ASSERT(false, ("Assume that we have valid feature always"));
    break;
  }

  return s;
}

string DebugPrint(FeatureType const & ft)
{
  return ft.DebugString(FeatureType::BEST_GEOMETRY);
}

bool FeatureType::IsEmptyGeometry(int scale) const
{
  ParseGeometryAndTriangles(scale);

  switch (GetFeatureType())
  {
  case GEOM_AREA: return m_triangles.empty();
  case GEOM_LINE: return m_points.empty();
  default: return false;
  }
}

m2::RectD FeatureType::GetLimitRect(int scale) const
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

void FeatureType::ParseGeometryAndTriangles(int scale) const
{
  ParseGeometry(scale);
  ParseTriangles(scale);
}

FeatureType::geom_stat_t FeatureType::GetGeometrySize(int scale) const
{
  uint32_t sz = ParseGeometry(scale);
  if (sz == 0 && !m_points.empty())
    sz = m_innerStats.m_points;

  return geom_stat_t(sz, m_points.size());
}

FeatureType::geom_stat_t FeatureType::GetTrianglesSize(int scale) const
{
  uint32_t sz = ParseTriangles(scale);
  if (sz == 0 && !m_triangles.empty())
    sz = m_innerStats.m_strips;

  return geom_stat_t(sz, m_triangles.size());
}

void FeatureType::GetPreferredNames(string & primary, string & secondary) const
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

void FeatureType::GetPreferredNames(bool allowTranslit, int8_t deviceLang, string & primary, string & secondary) const
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

void FeatureType::GetReadableName(string & name) const
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

void FeatureType::GetReadableName(bool allowTranslit, int8_t deviceLang, string & name) const
{
  if (!HasName())
    return;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  ParseCommon();

  ::GetReadableName(mwmInfo->GetRegionData(), GetNames(), deviceLang, allowTranslit, name);
}

string FeatureType::GetHouseNumber() const
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

bool FeatureType::GetName(int8_t lang, string & name) const
{
  if (!HasName())
    return false;

  ParseCommon();
  return m_params.name.GetString(lang, name);
}

uint8_t FeatureType::GetRank() const
{
  ParseCommon();
  return m_params.rank;
}

uint64_t FeatureType::GetPopulation() const
{
  return feature::RankToPopulation(GetRank());
}

string FeatureType::GetRoadNumber() const
{
  ParseCommon();
  return m_params.ref;
}

void FeatureType::SwapGeometry(FeatureType & r)
{
  ASSERT_EQUAL(m_pointsParsed, r.m_pointsParsed, ());
  ASSERT_EQUAL(m_trianglesParsed, r.m_trianglesParsed, ());

  if (m_pointsParsed)
    m_points.swap(r.m_points);

  if (m_trianglesParsed)
    m_triangles.swap(r.m_triangles);
}
