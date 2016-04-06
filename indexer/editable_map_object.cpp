#include "indexer/classificator.hpp"
#include "indexer/cuisines.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

namespace osm
{
bool EditableMapObject::IsNameEditable() const { return m_editableProperties.m_name; }
bool EditableMapObject::IsAddressEditable() const { return m_editableProperties.m_address; }

bool EditableMapObject::ShouldDisplayAddOrganisationButton() const
{
  return ftypes::IsBuildingChecker::Instance()(m_types);
}

vector<Props> EditableMapObject::GetEditableProperties() const
{
  return MetadataToProps(m_editableProperties.m_metadata);
}

vector<feature::Metadata::EType> const & EditableMapObject::GetEditableFields() const
{
  return m_editableProperties.m_metadata;
}

StringUtf8Multilang const & EditableMapObject::GetName() const { return m_name; }

vector<LocalizedName> EditableMapObject::GetLocalizedNames() const
{
  vector<LocalizedName> result;
  m_name.ForEach([&result](int8_t code, string const & name) -> bool
                 {
                   result.push_back({code, StringUtf8Multilang::GetLangByCode(code),
                                     StringUtf8Multilang::GetLangNameByCode(code), name});
                   return true;
                 });
  return result;
}

vector<string> const & EditableMapObject::GetNearbyStreets() const { return m_nearbyStreets; }
string const & EditableMapObject::GetHouseNumber() const { return m_houseNumber; }

string EditableMapObject::GetPostcode() const
{
  return m_metadata.Get(feature::Metadata::FMD_POSTCODE);
}

string EditableMapObject::GetWikipedia() const
{
  return m_metadata.Get(feature::Metadata::FMD_WIKIPEDIA);
}

void EditableMapObject::SetEditableProperties(osm::EditableProperties const & props)
{
  m_editableProperties = props;
}

void EditableMapObject::SetName(StringUtf8Multilang const & name) { m_name = name; }

void EditableMapObject::SetName(string const & name, int8_t langCode)
{
  if (!name.empty())
    m_name.AddString(langCode, name);
}

void EditableMapObject::SetMercator(m2::PointD const & center) { m_mercator = center; }

void EditableMapObject::SetType(uint32_t featureType)
{
  if (m_types.GetGeoType() == feature::EGeomType::GEOM_UNDEFINED)
  {
    // Support only point type for newly created features.
    m_types = feature::TypesHolder(feature::EGeomType::GEOM_POINT);
    m_types.Assign(featureType);
  }
  else
  {
    // Correctly replace "main" type in cases when feature holds more types.
    ASSERT(!m_types.Empty(), ());
    feature::TypesHolder copy = m_types;
    // TODO(mgsergio): Replace by correct sorting from editor's config.
    copy.SortBySpec();
    m_types.Remove(*copy.begin());
    m_types.operator ()(featureType);
  }
}

void EditableMapObject::SetID(FeatureID const & fid) { m_featureID = fid; }
void EditableMapObject::SetStreet(string const & street) { m_street = street; }

void EditableMapObject::SetNearbyStreets(vector<string> && streets)
{
  m_nearbyStreets = move(streets);
}
void EditableMapObject::SetHouseNumber(string const & houseNumber)
{
  // TODO(AlexZ): Check house number for validity with feature::IsHouseNumber ?
  // TODO(AlexZ): Store edited house number as house name if feature::IsHouseNumber() returned false.
  m_houseNumber = houseNumber;
}
void EditableMapObject::SetPostcode(string const & postcode)
{
  m_metadata.Set(feature::Metadata::FMD_POSTCODE, postcode);
}

void EditableMapObject::SetPhone(string const & phone)
{
  m_metadata.Set(feature::Metadata::FMD_PHONE_NUMBER, phone);
}

void EditableMapObject::SetFax(string const & fax)
{
  m_metadata.Set(feature::Metadata::FMD_FAX_NUMBER, fax);
}

void EditableMapObject::SetEmail(string const & email)
{
  m_metadata.Set(feature::Metadata::FMD_EMAIL, email);
}

void EditableMapObject::SetWebsite(string const & website)
{
  m_metadata.Set(feature::Metadata::FMD_WEBSITE, website);
  m_metadata.Drop(feature::Metadata::FMD_URL);
}

void EditableMapObject::SetInternet(Internet internet)
{
  m_metadata.Set(feature::Metadata::FMD_INTERNET, DebugPrint(internet));
}

void EditableMapObject::SetStars(int stars)
{
  if (stars > 0 && stars <= 7)
    m_metadata.Set(feature::Metadata::FMD_STARS, strings::to_string(stars));
  else
    LOG(LWARNING, ("Ignored invalid value to Stars:", stars));
}

void EditableMapObject::SetOperator(string const & op)
{
  m_metadata.Set(feature::Metadata::FMD_OPERATOR, op);
}

void EditableMapObject::SetElevation(double ele)
{
  // TODO: Reuse existing validadors in generator (osm2meta).
  constexpr double kMaxElevationOnTheEarthInMeters = 10000;
  constexpr double kMinElevationOnTheEarthInMeters = -15000;
  if (ele < kMaxElevationOnTheEarthInMeters && ele > kMinElevationOnTheEarthInMeters)
    m_metadata.Set(feature::Metadata::FMD_ELE, strings::to_string_dac(ele, 1));
  else
    LOG(LWARNING, ("Ignored invalid value to Elevation:", ele));
}

void EditableMapObject::SetWikipedia(string const & wikipedia)
{
  m_metadata.Set(feature::Metadata::FMD_WIKIPEDIA, wikipedia);
}

void EditableMapObject::SetFlats(string const & flats)
{
  m_metadata.Set(feature::Metadata::FMD_FLATS, flats);
}

void EditableMapObject::SetBuildingLevels(string const & buildingLevels)
{
  auto constexpr kMaximumLevelsEditableByUsers = 50;
  uint64_t levels;
  if (strings::to_uint64(buildingLevels, levels) && levels <= kMaximumLevelsEditableByUsers)
    m_metadata.Set(feature::Metadata::FMD_BUILDING_LEVELS, buildingLevels);
}

string const & EditableMapObject::GetStreet() const { return m_street; }

void EditableMapObject::SetCuisines(vector<string> const & cuisine)
{
  m_metadata.Set(feature::Metadata::FMD_CUISINE, strings::JoinStrings(cuisine, ';'));
}

void EditableMapObject::SetOpeningHours(string const & openingHours)
{
  m_metadata.Set(feature::Metadata::FMD_OPEN_HOURS, openingHours);
}

void EditableMapObject::SetPointType() { m_geomType = feature::EGeomType::GEOM_POINT; }
}  // namespace osm
