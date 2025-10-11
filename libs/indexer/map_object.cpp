#include "map_object.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"

#include "base/string_utils.hpp"

namespace osm
{
using namespace std;

// FeatureNames -------------------------------------------------------------------------------

void FeatureNames::Add(int8_t langCode, std::string_view name)
{
  ASSERT(langCode != StringUtf8Multilang::kUnsupportedLanguageCode, ());

  strings::Trim(name);

  if (name.empty())
    m_str.RemoveString(langCode);
  else
    m_str.AddString(langCode, name);
}

void FeatureNames::Add(std::string_view lang, std::string_view name)
{
  Add(StringUtf8Multilang::GetLangIndex(lang), name);
}

std::string_view FeatureNames::Get(int8_t langCode) const
{
  ASSERT(langCode != StringUtf8Multilang::kUnsupportedLanguageCode, ());

  std::string_view res;
  m_str.GetString(langCode, res);
  return res;
}

// MapObject -------------------------------------------------------------------------------

void MapObject::SetFromFeatureType(FeatureType & ft)
{
  m_mercator = feature::GetCenter(ft);
  m_name.FromBuffer(ft.GetNames());

  Classificator const & cl = classif();
  m_types = feature::TypesHolder(ft);
  m_types.RemoveIf([&cl](uint32_t t) { return !cl.IsTypeValid(t); });
  // Actually, we can't select object on map with invalid (non-drawable or deprecated) type.
  // TODO: in android prod a user will see an "empty" PP if a spot is selected in old mwm
  // where a deprecated feature was; and could crash if play with routing to it, bookmarking it..
  // A desktop/qt prod segfaults when trying to select such spots.
  ASSERT(!m_types.Empty(), ());

  m_metadata = ft.GetMetadata();
  m_houseNumber = ft.GetHouseNumber();
  m_roadShields = ftypes::GetRoadShieldsNames(ft);
  m_featureID = ft.GetID();
  m_geomType = ft.GetGeomType();
  m_layer = ft.GetLayer();

  // TODO: BEST_GEOMETRY is likely needed for some special cases only,
  // i.e. matching an edited OSM feature, in other cases like opening
  // a place page WORST_GEOMETRY is going to be enough?
  if (m_geomType == feature::GeomType::Area)
    assign_range(m_triangles, ft.GetTrianglesAsPoints(FeatureType::BEST_GEOMETRY));
  else if (m_geomType == feature::GeomType::Line)
    assign_range(m_points, ft.GetPoints(FeatureType::BEST_GEOMETRY));

  // Fill runtime metadata
  m_metadata.Set(feature::Metadata::EType::FMD_WHEELCHAIR, feature::GetReadableWheelchairType(m_types));

#ifdef DEBUG
  if (ftypes::IsWifiChecker::Instance()(ft))
    ASSERT(m_metadata.Has(MetadataID::FMD_INTERNET), ());
#endif
}

ms::LatLon MapObject::GetLatLon() const
{
  return mercator::ToLatLon(m_mercator);
}

string_view MapObject::GetDefaultName() const
{
  return m_name.Get(StringUtf8Multilang::kDefaultCode);
}

std::string_view MapObject::GetPostcode() const
{
  return m_metadata.Get(MetadataID::FMD_POSTCODE);
}

std::string MapObject::GetLocalizedType() const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();

  return platform::GetLocalizedTypeName(classif().GetReadableObjectName(copy.GetBestType()));
}

std::string MapObject::GetLocalizedAllTypes(bool withMainType) const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();

  auto const & isPoi = ftypes::IsPoiChecker::Instance();
  auto const & amenityChecker = ftypes::IsAmenityChecker::Instance();

  std::ostringstream oss;
  bool isMainType = true;
  bool isFirst = true;
  for (auto const type : copy)
  {
    if (isMainType && !withMainType)
    {
      isMainType = false;
      continue;
    }

    // Ignore types that are not POI
    if (!isMainType && !isPoi(type))
      continue;

    // Ignore general amenity
    if (!isMainType && amenityChecker.GetType() == type)
      continue;

    isMainType = false;

    // Add fields separator between types
    if (isFirst)
      isFirst = false;
    else
      oss << feature::kFieldsSeparator;

    oss << platform::GetLocalizedTypeName(classif().GetReadableObjectName(type));
  }

  return oss.str();
}

std::string MapObject::GetAllReadableTypes() const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();

  std::ostringstream oss;

  for (auto const type : copy)
    oss << classif().GetReadableObjectName(type) << feature::kFieldsSeparator;

  return oss.str();
}

std::string_view MapObject::GetMetadata(MetadataID type) const
{
  return m_metadata.Get(type);
}

std::string_view MapObject::GetOpeningHours() const
{
  return m_metadata.Get(MetadataID::FMD_OPEN_HOURS);
}

feature::Internet MapObject::GetInternet() const
{
  return feature::InternetFromString(m_metadata.Get(MetadataID::FMD_INTERNET));
}

vector<string> MapObject::GetCuisines() const
{
  return feature::GetCuisines(m_types);
}

vector<string> MapObject::GetLocalizedCuisines() const
{
  return feature::GetLocalizedCuisines(m_types);
}

vector<string> MapObject::GetRecyclingTypes() const
{
  return feature::GetRecyclingTypes(m_types);
}

vector<string> MapObject::GetLocalizedRecyclingTypes() const
{
  return feature::GetLocalizedRecyclingTypes(m_types);
}

string MapObject::GetLocalizedFeeType() const
{
  return feature::GetLocalizedFeeType(m_types);
}

bool MapObject::HasAtm() const
{
  return feature::HasAtm(m_types);
}

bool MapObject::HasToilets() const
{
  return feature::HasToilets(m_types);
}

string MapObject::FormatCuisines() const
{
  return strings::JoinStrings(GetLocalizedCuisines(), feature::kFieldsSeparator);
}

string MapObject::FormatRoadShields() const
{
  return strings::JoinStrings(m_roadShields, feature::kFieldsSeparator);
}

int MapObject::GetStars() const
{
  uint8_t count = 0;

  auto const sv = m_metadata.Get(MetadataID::FMD_STARS);
  if (!sv.empty())
  {
    if (!strings::to_uint(sv, count))
      count = 0;
  }

  return count;
}

bool MapObject::IsPointType() const
{
  return m_geomType == feature::GeomType::Point;
}
bool MapObject::IsBuilding() const
{
  return ftypes::IsBuildingChecker::Instance()(m_types);
}
bool MapObject::IsPublicTransportStop() const
{
  return ftypes::IsPublicTransportStopChecker::Instance()(m_types);
}

bool MapObject::HasSchedule() const
{
  using namespace ftypes;
  return IsPublicTransportStopChecker::Instance()(m_types) || IsRailwayStationChecker::Instance()(m_types) ||
         IsSubwayStationChecker::Instance()(m_types);
}

std::string DebugPrint(MapObject const & mo)
{
  return DebugPrint(mo.m_featureID);
}

}  // namespace osm
