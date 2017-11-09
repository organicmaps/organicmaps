#include "map_object.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/cuisines.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace osm
{
namespace
{
constexpr char const * kWlan = "wlan";
constexpr char const * kWired = "wired";
constexpr char const * kYes = "yes";
constexpr char const * kNo = "no";

void SetInetIfNeeded(FeatureType const & ft, feature::Metadata & metadata)
{
  if (!ftypes::IsWifiChecker::Instance()(ft) || metadata.Has(feature::Metadata::FMD_INTERNET))
    return;

  metadata.Set(feature::Metadata::FMD_INTERNET, kWlan);
}
}  // namespace

string DebugPrint(osm::Internet internet)
{
  switch (internet)
  {
  case Internet::No: return kNo;
  case Internet::Yes: return kYes;
  case Internet::Wlan: return kWlan;
  case Internet::Wired: return kWired;
  case Internet::Unknown: break;
  }
  return {};
}

string DebugPrint(Props props)
{
  string k;
  switch (props)
  {
  case osm::Props::Phone: k = "phone"; break;
  case osm::Props::Fax: k = "fax"; break;
  case osm::Props::Email: k = "email"; break;
  case osm::Props::Website: k = "website"; break;
  case osm::Props::Internet: k = "internet_access"; break;
  case osm::Props::Cuisine: k = "cuisine"; break;
  case osm::Props::OpeningHours: k = "opening_hours"; break;
  case osm::Props::Stars: k = "stars"; break;
  case osm::Props::Operator: k = "operator"; break;
  case osm::Props::Elevation: k = "ele"; break;
  case osm::Props::Wikipedia: k = "wikipedia"; break;
  case osm::Props::Flats: k = "addr:flats"; break;
  case osm::Props::BuildingLevels: k = "building:levels"; break;
  case osm::Props::Level: k = "level"; break;
  }
  return k;
}

void MapObject::SetFromFeatureType(FeatureType const & ft)
{
  m_mercator = feature::GetCenter(ft);
  m_name = ft.GetNames();
  m_types = feature::TypesHolder(ft);
  m_metadata = ft.GetMetadata();
  m_featureID = ft.GetID();
  ASSERT(m_featureID.IsValid(), ());
  m_geomType = ft.GetFeatureType();

  SetInetIfNeeded(ft, m_metadata);
}

FeatureID const & MapObject::GetID() const { return m_featureID; }
ms::LatLon MapObject::GetLatLon() const { return MercatorBounds::ToLatLon(m_mercator); }
m2::PointD const & MapObject::GetMercator() const { return m_mercator; }
feature::TypesHolder const & MapObject::GetTypes() const { return m_types; }

string MapObject::GetDefaultName() const
{
  string name;
  UNUSED_VALUE(m_name.GetString(StringUtf8Multilang::kDefaultCode, name));
  return name;
}

string MapObject::GetLocalizedType() const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();
  CategoriesHolder const & categories = GetDefaultCategories();
  return categories.GetReadableFeatureType(
      *copy.begin(), categories.MapLocaleToInteger(languages::GetCurrentOrig()));
}

vector<osm::Props> MapObject::AvailableProperties() const
{
  return MetadataToProps(m_metadata.GetPresentTypes());
}

string MapObject::GetPhone() const { return m_metadata.Get(feature::Metadata::FMD_PHONE_NUMBER); }
string MapObject::GetFax() const { return m_metadata.Get(feature::Metadata::FMD_FAX_NUMBER); }
string MapObject::GetEmail() const { return m_metadata.Get(feature::Metadata::FMD_EMAIL); }

string MapObject::GetWebsite() const
{
  string website = m_metadata.Get(feature::Metadata::FMD_WEBSITE);
  if (website.empty())
    website = m_metadata.Get(feature::Metadata::FMD_URL);
  return website;
}

Internet MapObject::GetInternet() const
{
  string inet = m_metadata.Get(feature::Metadata::FMD_INTERNET);
  strings::AsciiToLower(inet);
  // Most popular case.
  if (inet.empty())
    return Internet::Unknown;
  if (inet.find(kWlan) != string::npos)
    return Internet::Wlan;
  if (inet.find(kWired) != string::npos)
    return Internet::Wired;
  if (inet == kYes)
    return Internet::Yes;
  if (inet == kNo)
    return Internet::No;
  return Internet::Unknown;
}

vector<string> MapObject::GetCuisines() const
{
  vector<string> cuisines;
  Cuisines::Instance().Parse(m_metadata.Get(feature::Metadata::FMD_CUISINE), cuisines);
  return cuisines;
}

vector<string> MapObject::GetLocalizedCuisines() const
{
  vector<string> localized;
  Cuisines::Instance().ParseAndLocalize(m_metadata.Get(feature::Metadata::FMD_CUISINE), localized);
  return localized;
}

string MapObject::FormatCuisines() const { return strings::JoinStrings(GetLocalizedCuisines(), " • "); }

string MapObject::GetOpeningHours() const
{
  return m_metadata.Get(feature::Metadata::FMD_OPEN_HOURS);
}

string MapObject::GetOperator() const { return m_metadata.Get(feature::Metadata::FMD_OPERATOR); }

int MapObject::GetStars() const
{
  // Most popular case.
  if (m_metadata.Has(feature::Metadata::FMD_STARS))
  {
    int count;
    if (strings::to_int(m_metadata.Get(feature::Metadata::FMD_STARS), count))
      return count;
  }
  return 0;
}

string MapObject::GetElevationFormatted() const
{
  if (m_metadata.Has(feature::Metadata::FMD_ELE))
  {
    double value;
    if (strings::to_double(m_metadata.Get(feature::Metadata::FMD_ELE), value))
      return measurement_utils::FormatAltitude(value);
    else
      LOG(LWARNING,
          ("Invalid metadata for elevation:", m_metadata.Get(feature::Metadata::FMD_ELE)));
  }
  return {};
}

bool MapObject::GetElevation(double & outElevationInMeters) const
{
  return strings::to_double(m_metadata.Get(feature::Metadata::FMD_ELE), outElevationInMeters);
}

string MapObject::GetWikipediaLink() const { return m_metadata.GetWikiURL(); }

string MapObject::GetFlats() const { return m_metadata.Get(feature::Metadata::FMD_FLATS); }

string MapObject::GetLevel() const { return m_metadata.Get(feature::Metadata::FMD_LEVEL); }

string MapObject::GetBuildingLevels() const
{
  return m_metadata.Get(feature::Metadata::FMD_BUILDING_LEVELS);
}

ftraits::WheelchairAvailability MapObject::GetWheelchairType() const
{
  auto const opt = ftraits::Wheelchair::GetValue(m_types);
  return opt ? *opt : ftraits::WheelchairAvailability::No;
}

feature::Metadata const & MapObject::GetMetadata() const { return m_metadata; }
bool MapObject::IsPointType() const { return m_geomType == feature::EGeomType::GEOM_POINT; }
bool MapObject::IsBuilding() const { return ftypes::IsBuildingChecker::Instance()(m_types); }

}  // namespace osm
