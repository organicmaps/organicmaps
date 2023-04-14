#include "map_object.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace osm
{
using namespace std;

namespace
{
constexpr char const * kWlan = "wlan";
constexpr char const * kWired = "wired";
constexpr char const * kTerminal = "terminal";
constexpr char const * kYes = "yes";
constexpr char const * kNo = "no";
}  // namespace

char const * MapObject::kFieldsSeparator = " • ";

string DebugPrint(osm::Internet internet)
{
  switch (internet)
  {
  case Internet::No: return kNo;
  case Internet::Yes: return kYes;
  case Internet::Wlan: return kWlan;
  case Internet::Wired: return kWired;
  case Internet::Terminal: return kTerminal;
  case Internet::Unknown: break;
  }
  return {};
}

void MapObject::SetFromFeatureType(FeatureType & ft)
{
  m_mercator = feature::GetCenter(ft);
  m_name = ft.GetNames();

  Classificator const & cl = classif();
  m_types = feature::TypesHolder(ft);
  m_types.RemoveIf([&cl](uint32_t t)
  {
    return !cl.IsTypeValid(t);
  });
  // Actually, we can't select object on map with invalid (non-drawable type).
  ASSERT(!m_types.Empty(), ());

  m_metadata = ft.GetMetadata();
  m_houseNumber = ft.GetHouseNumber();
  m_roadNumber = ft.GetRoadNumber();
  m_featureID = ft.GetID();
  m_geomType = ft.GetGeomType();

  if (m_geomType == feature::GeomType::Area)
    assign_range(m_triangles, ft.GetTrianglesAsPoints(FeatureType::BEST_GEOMETRY));
  else if (m_geomType == feature::GeomType::Line)
    assign_range(m_points, ft.GetPoints(FeatureType::BEST_GEOMETRY));

#ifdef DEBUG
  if (ftypes::IsWifiChecker::Instance()(ft))
    ASSERT(m_metadata.Has(MetadataID::FMD_INTERNET), ());
#endif
}

FeatureID const & MapObject::GetID() const { return m_featureID; }
ms::LatLon MapObject::GetLatLon() const { return mercator::ToLatLon(m_mercator); }
m2::PointD const & MapObject::GetMercator() const { return m_mercator; }
vector<m2::PointD> const & MapObject::GetTriangesAsPoints() const { return m_triangles; }
vector<m2::PointD> const & MapObject::GetPoints() const { return m_points; }
feature::TypesHolder const & MapObject::GetTypes() const { return m_types; }

string_view MapObject::GetDefaultName() const
{
  string_view name;
  UNUSED_VALUE(m_name.GetString(StringUtf8Multilang::kDefaultCode, name));
  return name;
}

StringUtf8Multilang const & MapObject::GetNameMultilang() const
{
  return m_name;
}

string const & MapObject::GetHouseNumber() const { return m_houseNumber; }

std::string_view MapObject::GetPostcode() const
{
  return m_metadata.Get(MetadataID::FMD_POSTCODE);
}

string MapObject::GetLocalizedType() const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();

  return platform::GetLocalizedTypeName(classif().GetReadableObjectName(copy.GetBestType()));
}

std::string_view MapObject::GetMetadata(MetadataID type) const
{
  return m_metadata.Get(type);
}

Internet InternetFromString(std::string_view inet)
{
  if (inet.empty())
    return Internet::Unknown;
  if (inet.find(kWlan) != string::npos)
    return Internet::Wlan;
  if (inet.find(kWired) != string::npos)
    return Internet::Wired;
  if (inet.find(kTerminal) != string::npos)
    return Internet::Terminal;
  if (inet == kYes)
    return Internet::Yes;
  if (inet == kNo)
    return Internet::No;
  return Internet::Unknown;
}

std::string_view MapObject::GetOpeningHours() const
{
  return m_metadata.Get(MetadataID::FMD_OPEN_HOURS);
}

Internet MapObject::GetInternet() const
{
  return InternetFromString(m_metadata.Get(MetadataID::FMD_INTERNET));
}

vector<string> MapObject::GetCuisines() const { return feature::GetCuisines(m_types); }

vector<string> MapObject::GetLocalizedCuisines() const
{
  return feature::GetLocalizedCuisines(m_types);
}

vector<string> MapObject::GetRecyclingTypes() const { return feature::GetRecyclingTypes(m_types); }

vector<string> MapObject::GetLocalizedRecyclingTypes() const
{
  return feature::GetLocalizedRecyclingTypes(m_types);
}

string MapObject::FormatCuisines() const
{
  return strings::JoinStrings(GetLocalizedCuisines(), kFieldsSeparator);
}

vector<string> MapObject::GetRoadShields() const
{
  return feature::GetRoadShieldsNames(m_roadNumber);
}

string MapObject::FormatRoadShields() const
{
  return strings::JoinStrings(GetRoadShields(), kFieldsSeparator);
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

string MapObject::GetElevationFormatted() const
{
  auto const sv = m_metadata.Get(MetadataID::FMD_ELE);
  if (!sv.empty())
  {
    double value;
    if (strings::to_double(sv, value))
      return measurement_utils::FormatAltitude(value);
    else
      LOG(LWARNING, ("Invalid elevation metadata:", sv));
  }
  return {};
}

ftraits::WheelchairAvailability MapObject::GetWheelchairType() const
{
  auto const opt = ftraits::Wheelchair::GetValue(m_types);
  return opt ? *opt : ftraits::WheelchairAvailability::No;
}

bool MapObject::IsPointType() const { return m_geomType == feature::GeomType::Point; }
bool MapObject::IsBuilding() const { return ftypes::IsBuildingChecker::Instance()(m_types); }

}  // namespace osm
