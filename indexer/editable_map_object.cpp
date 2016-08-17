#include "indexer/editable_map_object.hpp"
#include "indexer/classificator.hpp"
#include "indexer/cuisines.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/postcodes_matcher.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/cctype.hpp"
#include "std/cmath.hpp"


namespace
{
bool ExtractName(StringUtf8Multilang const & names, int8_t const langCode,
                 vector<osm::LocalizedName> & result)
{
  if (StringUtf8Multilang::kUnsupportedLanguageCode == langCode ||
      StringUtf8Multilang::kDefaultCode == langCode)
  {
    return false;
  }

  // Exclude languages that are already present.
  auto const it =
      find_if(result.begin(), result.end(), [langCode](osm::LocalizedName const & localizedName) {
        return localizedName.m_code == langCode;
      });

  if (result.end() != it)
    return false;
  
  string name;
  names.GetString(langCode, name);
  result.emplace_back(langCode, name);

  return true;
}

size_t PushMwmLanguages(StringUtf8Multilang const & names, vector<int8_t> const & mwmLanguages,
                        vector<osm::LocalizedName> & result)
{
  size_t count = 0;
  static size_t const kMaxCountMwmLanguages = 2;

  for (size_t i = 0; i < mwmLanguages.size() && count < kMaxCountMwmLanguages; ++i)
  {
    if (ExtractName(names, mwmLanguages[i], result))
      ++count;
  }
  
  return count;
}

string const kWebSiteProtocols[] = {"http://", "https://"};
string const kWebSiteProtocolByDefault = "http://";

bool IsProtocolSpecified(string const & website)
{
  for (auto const & protocol : kWebSiteProtocols)
  {
    if (strings::StartsWith(website, protocol.c_str()))
      return true;
  }
  return false;
}
}  // namespace

namespace osm
{
// LocalizedName -----------------------------------------------------------------------------------

LocalizedName::LocalizedName(int8_t const code, string const & name)
  : m_code(code)
  , m_lang(StringUtf8Multilang::GetLangByCode(code))
  , m_langName(StringUtf8Multilang::GetLangNameByCode(code))
  , m_name(name)
{
}

LocalizedName::LocalizedName(string const & langCode, string const & name)
  : m_code(StringUtf8Multilang::GetLangIndex(langCode))
  , m_lang(StringUtf8Multilang::GetLangByCode(m_code))
  , m_langName(StringUtf8Multilang::GetLangNameByCode(m_code))
  , m_name(name)
{
}

// EditableMapObject -------------------------------------------------------------------------------

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

NamesDataSource EditableMapObject::GetNamesDataSource() const
{
  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  if (!mwmInfo)
    return NamesDataSource();

  vector<int8_t> mwmLanguages;
  mwmInfo->GetRegionData().GetLanguages(mwmLanguages);

  auto const userLangCode = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());

  return GetNamesDataSource(m_name, mwmLanguages, userLangCode);
}

// static
NamesDataSource EditableMapObject::GetNamesDataSource(StringUtf8Multilang const & source,
                                                      vector<int8_t> const & mwmLanguages,
                                                      int8_t const userLangCode)
{
  NamesDataSource result;
  auto & names = result.names;
  auto & mandatoryCount = result.mandatoryNamesCount;
  // Push Mwm languages.
  mandatoryCount = PushMwmLanguages(source, mwmLanguages, names);

  // Push english name.
  if (ExtractName(source, StringUtf8Multilang::kEnglishCode, names))
    ++mandatoryCount;
  
  // Push user's language.
  if (ExtractName(source, userLangCode, names))
    ++mandatoryCount;

  // Push other languages.
  source.ForEach([&names, mandatoryCount](int8_t const code, string const & name) -> bool {
    // Exclude default name.
    if (StringUtf8Multilang::kDefaultCode == code)
      return true;
    
    auto const mandatoryNamesEnd = names.begin() + mandatoryCount;
    // Exclude languages which are already in container (languages with top priority).
    auto const it = find_if(
        names.begin(), mandatoryNamesEnd,
        [code](LocalizedName const & localizedName) { return localizedName.m_code == code; });

    if (mandatoryNamesEnd == it)
      names.emplace_back(code, name);
    
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

void EditableMapObject::SetName(string name, int8_t langCode)
{
  strings::Trim(name);
  if (name.empty())
    return;

  ASSERT_NOT_EQUAL(StringUtf8Multilang::kDefaultCode, langCode,
                   ("Direct editing of default name is deprecated."));

  if (!Editor::Instance().OriginalFeatureHasDefaultName(GetID()))
  {
    const auto mwmInfo = GetID().m_mwmId.GetInfo();

    if (mwmInfo)
    {
      vector<int8_t> mwmLanguages;
      mwmInfo->GetRegionData().GetLanguages(mwmLanguages);

      if (CanUseAsDefaultName(langCode, mwmLanguages))
        m_name.AddString(StringUtf8Multilang::kDefaultCode, name);
    }
  }

  m_name.AddString(langCode, name);
}

// static
bool EditableMapObject::CanUseAsDefaultName(int8_t const lang, vector<int8_t> const & mwmLanguages)
{
  for (auto const & mwmLang : mwmLanguages)
  {
    if (StringUtf8Multilang::kUnsupportedLanguageCode == mwmLang)
      continue;

    if (lang == mwmLang)
      return true;
  }

  return false;
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
  if (!website.empty() && !IsProtocolSpecified(website))
    website = kWebSiteProtocolByDefault + website;

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

  if ('0' == buildingLevels.front())
    return false;

  uint64_t levels;
  return strings::to_uint64(buildingLevels, levels) && levels > 0 && levels <= kMaximumLevelsEditableByUsers;
}

// static
bool EditableMapObject::ValidateHouseNumber(string const & houseNumber)
{
  // TODO(mgsergio): Use LooksLikeHouseNumber!

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

  size_t startPos = 0;
  for (auto const & protocol : kWebSiteProtocols)
  {
    if (strings::StartsWith(site, protocol.c_str()))
    {
      startPos = protocol.size();
      break;
    }
  }

  if (startPos >= site.size())
    return false;

  // Site should contain at least one dot but not at the begining/end.
  if ('.' == site[startPos] || '.' == site.back())
    return false;

  if (string::npos == site.find("."))
    return false;

  if (string::npos != site.find(".."))
    return false;

  return true;
}

// static
bool EditableMapObject::ValidateEmail(string const & email)
{
  if (email.empty())
    return true;

  if (strings::IsASCIIString(email))
    return regex_match(email, regex(R"([^@\s]+@[a-zA-Z0-9-]+(\.[a-zA-Z0-9-]+)+$)"));

  if ('@' == email.front() || '@' == email.back())
    return false;

  if ('.' == email.back())
    return false;

  auto const atPos = find(begin(email), end(email), '@');
  if (atPos == end(email))
    return false;

  // There should be only one '@' sign.
  if (find(next(atPos), end(email), '@') != end(email))
    return false;

  // There should be at least one '.' sign after '@'
  if (find(next(atPos), end(email), '.') == end(email))
    return false;

  return true;
}
}  // namespace osm
