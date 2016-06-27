#include "indexer/classificator.hpp"
#include "indexer/cuisines.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/postcodes_matcher.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/cctype.hpp"
#include "std/cmath.hpp"

namespace osm
{
// static
int8_t const EditableMapObject::kMaximumLevelsEditableByUsers = 25;

bool EditableMapObject::IsNameEditable() const { return m_editableProperties.m_name; }
bool EditableMapObject::IsAddressEditable() const { return m_editableProperties.m_address; }

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

vector<LocalizedStreet> const & EditableMapObject::GetNearbyStreets() const { return m_nearbyStreets; }
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
void EditableMapObject::SetStreet(LocalizedStreet const & st) { m_street = st; }

void EditableMapObject::SetNearbyStreets(vector<LocalizedStreet> && streets)
{
  m_nearbyStreets = move(streets);
}

void EditableMapObject::SetHouseNumber(string const & houseNumber)
{
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

void EditableMapObject::SetWebsite(string website)
{
  if (!website.empty() &&
      !strings::StartsWith(website, "http://") &&
      !strings::StartsWith(website, "https://"))
  {
    website = "http://" + website;
  }
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
  m_metadata.Set(feature::Metadata::FMD_BUILDING_LEVELS, buildingLevels);
}

LocalizedStreet const & EditableMapObject::GetStreet() const { return m_street; }

void EditableMapObject::SetCuisines(vector<string> const & cuisine)
{
  m_metadata.Set(feature::Metadata::FMD_CUISINE, strings::JoinStrings(cuisine, ';'));
}

void EditableMapObject::SetOpeningHours(string const & openingHours)
{
  m_metadata.Set(feature::Metadata::FMD_OPEN_HOURS, openingHours);
}

void EditableMapObject::SetPointType() { m_geomType = feature::EGeomType::GEOM_POINT; }

// static
bool EditableMapObject::ValidateBuildingLevels(string const & buildingLevels)
{
  if (buildingLevels.empty())
    return true;

  if (buildingLevels.size() > 18 /* max number of digits in uint_64 */)
    return false;

  uint64_t levels;
  return strings::to_uint64(buildingLevels, levels) && levels > 0 && levels <= kMaximumLevelsEditableByUsers;
}

// static
bool EditableMapObject::ValidateHouseNumber(string const & houseNumber)
{
  // TODO(mgsergio): Make a better validation, use real samples for example.

  if (houseNumber.empty())
    return true;

  strings::UniString us = strings::MakeUniString(houseNumber);
  // TODO: Improve this basic limit - it was choosen by @Zverik.
  auto constexpr kMaxHouseNumberLength = 15;
  if (us.size() > kMaxHouseNumberLength)
    return false;

  // TODO: Should we allow arabic numbers like U+0661 ١	Arabic-Indic Digit One?
  strings::NormalizeDigits(us);
  for (auto const c : us)
  {
    // Valid house numbers contain at least one digit.
    if (strings::IsASCIIDigit(c))
      return true;
  }
  return false;
}

// static
bool EditableMapObject::ValidateFlats(string const & flats)
{
  for (auto it = strings::SimpleTokenizer(flats, ";"); it; ++it)
  {
    auto token = *it;
    strings::Trim(token);

    vector<string> range;
    for (auto i = strings::SimpleTokenizer(token, "-"); i; ++i)
      range.push_back(*i);
    if (range.empty() || range.size() > 2)
      return false;

    for (auto const & rangeBorder : range)
    {
      if (!all_of(begin(rangeBorder), end(rangeBorder), isalnum))
        return false;
    }
  }
  return true;
}

// static
bool EditableMapObject::ValidatePostCode(string const & postCode)
{
  if (postCode.empty())
    return true;
  return search::LooksLikePostcode(postCode, false /* IsPrefix */);
}

// static
bool EditableMapObject::ValidatePhone(string const & phone)
{
  if (phone.empty())
    return true;

  auto curr = begin(phone);
  auto const last = end(phone);

  auto const kMaxNumberLen = 15;
  auto const kMinNumberLen = 5;

  if (*curr == '+')
    ++curr;

  auto digitsCount = 0;
  for (; curr != last; ++curr)
  {
    auto const isCharValid = isdigit(*curr) || *curr == '(' ||
                             *curr == ')' || *curr == ' ' || *curr == '-';
    if (!isCharValid)
      return false;

    if (isdigit(*curr))
      ++digitsCount;
  }

  return kMinNumberLen <= digitsCount && digitsCount <= kMaxNumberLen;
}

// static
bool EditableMapObject::ValidateWebsite(string const & site)
{
  if (site.empty())
    return true;

  auto const dotPos = find(begin(site), end(site), '.');
  // Site should contain at least one dot but not at the begining/and.
  if (dotPos == end(site) || site.front() == '.' || site.back() == '.')
    return false;

  return true;
}

// static
bool EditableMapObject::ValidateEmail(string const & email)
{
  if (email.empty())
    return true;

  auto const atPos = find(begin(email), end(email), '@');
  if (atPos == end(email))
    return false;

  // There should be only one '@' sign.
  if (find(next(atPos), end(email), '@') != end(email))
    return false;

  // There should be at least one '.' sign after '@' ...
  if (find(next(atPos), end(email), '.') == end(email))
    return false;

  // ... not in the end.
  if (email.back() == '.')
    return false;

  return true;
}
}  // namespace osm
