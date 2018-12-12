#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"

#include "base/base.hpp"
#include "base/control_flow.hpp"

#include <unordered_map>
#include <utility>

using namespace std;

namespace
{
using StrUtf8 = StringUtf8Multilang;

int8_t GetIndex(string const & lang)
{
  return StrUtf8::GetLangIndex(lang);
}

unordered_map<int8_t, vector<int8_t>> const kSimilarToDeviceLanguages =
{
  {GetIndex("be"), {GetIndex("ru")}}
};

void GetMwmLangName(feature::RegionData const & regionData, StringUtf8Multilang const & src, string & out)
{
  vector<int8_t> mwmLangCodes;
  regionData.GetLanguages(mwmLangCodes);

  for (auto const code : mwmLangCodes)
  {
    if (src.GetString(code, out))
      return;
  }
}

bool GetTransliteratedName(feature::RegionData const & regionData, StringUtf8Multilang const & src, string & out)
{
  vector<int8_t> mwmLangCodes;
  regionData.GetLanguages(mwmLangCodes);

  string srcName;
  for (auto const code : mwmLangCodes)
    if (src.GetString(code, srcName) && Transliteration::Instance().Transliterate(srcName, code, out))
      return true;

  // If default name is available, interpret it as a name for the first mwm language.
  if (!mwmLangCodes.empty() && src.GetString(StringUtf8Multilang::kDefaultCode, srcName))
    return Transliteration::Instance().Transliterate(srcName, mwmLangCodes[0], out);

  return false;
}

bool GetBestName(StringUtf8Multilang const & src, vector<int8_t> const & priorityList, string & out)
{
  auto bestIndex = priorityList.size();

  auto const findAndSet = [](vector<int8_t> const & langs, int8_t const code, string const & name,
                             size_t & bestIndex, string & outName)
  {
    auto const it = find(langs.begin(), langs.end(), code);
    if (it != langs.end() && bestIndex > static_cast<size_t>(distance(langs.begin(), it)))
    {
      bestIndex = distance(langs.begin(), it);
      outName = name;
    }
  };

  src.ForEach([&](int8_t code, string const & name)
              {
                if (bestIndex == 0)
                  return base::ControlFlow::Break;

                findAndSet(priorityList, code, name, bestIndex, out);
                return base::ControlFlow::Continue;
              });

  // There are many "junk" names in Arabian island.
  if (bestIndex < priorityList.size() &&
    priorityList[bestIndex] == StrUtf8::kInternationalCode)
  {
    out = out.substr(0, out.find_first_of(','));
  }

  return bestIndex < priorityList.size();
}

vector<int8_t> GetSimilarToDeviceLanguages(int8_t deviceLang)
{  
  auto const it = kSimilarToDeviceLanguages.find(deviceLang);
  if (it != kSimilarToDeviceLanguages.cend())
    return it->second;

  return {};
}

bool IsNativeLang(feature::RegionData const & regionData, int8_t deviceLang)
{
  if (regionData.HasLanguage(deviceLang))
    return true;

  for (auto const lang : GetSimilarToDeviceLanguages(deviceLang))
  {
    if (regionData.HasLanguage(lang))
      return true;
  }

  return false;
}

vector<int8_t> MakePrimaryNamePriorityList(int8_t deviceLang, bool preferDefault)
{
  vector<int8_t> langPriority = {deviceLang};
  if (preferDefault)
    langPriority.push_back(StrUtf8::kDefaultCode);

  auto const similarLangs = GetSimilarToDeviceLanguages(deviceLang);
  langPriority.insert(langPriority.cend(), similarLangs.cbegin(), similarLangs.cend());
  langPriority.insert(langPriority.cend(), {StrUtf8::kInternationalCode, StrUtf8::kEnglishCode});

  return langPriority;
}

void GetReadableNameImpl(feature::RegionData const & regionData, StringUtf8Multilang const & src,
                         int8_t deviceLang, bool preferDefault, bool allowTranslit, string & out)
{
  vector<int8_t> langPriority = MakePrimaryNamePriorityList(deviceLang, preferDefault);

  if (GetBestName(src, langPriority, out))
    return;

  if (allowTranslit && GetTransliteratedName(regionData, src, out))
    return;

  if (!preferDefault)
  {
    if (GetBestName(src, {StrUtf8::kDefaultCode}, out))
      return;
  }

  GetMwmLangName(regionData, src, out);
}
}  // namespace

namespace feature
{
namespace impl
{
class FeatureEstimator
{
  template <size_t N>
  static bool IsEqual(uint32_t t, uint32_t const (&arr)[N])
  {
    for (size_t i = 0; i < N; ++i)
      if (arr[i] == t)
        return true;
    return false;
  }

  static bool InSubtree(uint32_t t, uint32_t const orig)
  {
    ftype::TruncValue(t, ftype::GetLevel(orig));
    return t == orig;
  }

public:

  FeatureEstimator()
  {
    m_TypeContinent   = GetType("place", "continent");
    m_TypeCountry     = GetType("place", "country");

    m_TypeState       = GetType("place", "state");
    m_TypeCounty[0]   = GetType("place", "region");
    m_TypeCounty[1]   = GetType("place", "county");

    m_TypeCity        = GetType("place", "city");
    m_TypeTown        = GetType("place", "town");

    m_TypeVillage[0]  = GetType("place", "village");
    m_TypeVillage[1]  = GetType("place", "suburb");

    m_TypeSmallVillage[0]  = GetType("place", "hamlet");
    m_TypeSmallVillage[1]  = GetType("place", "locality");
    m_TypeSmallVillage[2]  = GetType("place", "farm");
  }

  void CorrectScaleForVisibility(TypesHolder const & types, int & scale) const
  {
    pair<int, int> const scaleR = GetDrawableScaleRangeForRules(types, RULE_ANY_TEXT);
    ASSERT_LESS_OR_EQUAL ( scaleR.first, scaleR.second, () );

    // Result types can be without visible texts (matched by category).
    if (scaleR.first != -1)
    {
      if (scale < scaleR.first)
        scale = scaleR.first;
      else if (scale > scaleR.second)
        scale = scaleR.second;
    }
  }

  int GetViewportScale(TypesHolder const & types) const
  {
    int scale = GetDefaultScale();

    if (types.GetGeoType() == GEOM_POINT)
      for (uint32_t t : types)
        scale = min(scale, GetScaleForType(t));

    CorrectScaleForVisibility(types, scale);
    return scale;
  }

private:
  static int GetDefaultScale() { return scales::GetUpperComfortScale(); }

  // Returns width and height (lon and lat) for a given type.
  int GetScaleForType(uint32_t const type) const
  {
    if (type == m_TypeContinent)
      return 2;

    /// @todo Load countries bounding rects.
    if (type == m_TypeCountry)
      return 4;

    if (type == m_TypeState)
      return 6;

    if (IsEqual(type, m_TypeCounty))
      return 7;

    if (InSubtree(type, m_TypeCity))
      return 9;

    if (type == m_TypeTown)
      return 9;

    if (IsEqual(type, m_TypeVillage))
      return 12;

    if (IsEqual(type, m_TypeSmallVillage))
      return 14;

    return GetDefaultScale();
  }

  static uint32_t GetType(string const & s1,
                          string const & s2 = string(),
                          string const & s3 = string())
  {
    vector<string> path;
    path.push_back(s1);
    if (!s2.empty()) path.push_back(s2);
    if (!s3.empty()) path.push_back(s3);
    return classif().GetTypeByPath(path);
  }

  uint32_t m_TypeContinent;
  uint32_t m_TypeCountry;
  uint32_t m_TypeState;
  uint32_t m_TypeCounty[2];
  uint32_t m_TypeCity;
  uint32_t m_TypeTown;
  uint32_t m_TypeVillage[2];
  uint32_t m_TypeSmallVillage[3];
};

FeatureEstimator const & GetFeatureEstimator()
{
  static FeatureEstimator const featureEstimator;
  return featureEstimator;
}

}  // namespace feature::impl

int GetFeatureViewportScale(TypesHolder const & types)
{
  return impl::GetFeatureEstimator().GetViewportScale(types);
}

void GetPreferredNames(RegionData const & regionData, StringUtf8Multilang const & src,
                       int8_t const deviceLang, bool allowTranslit, string & primary, string & secondary)
{
  primary.clear();
  secondary.clear();

  if (src.IsEmpty())
    return;

  // When the language of the user is equal to one of the languages of the MWM
  // (or similar languages) only single name scheme is used.
  if (IsNativeLang(regionData, deviceLang))
    return GetReadableNameImpl(regionData, src, deviceLang, true, allowTranslit, primary);

  vector<int8_t> primaryCodes = MakePrimaryNamePriorityList(deviceLang, false);

  if (!GetBestName(src, primaryCodes, primary) && allowTranslit)
    GetTransliteratedName(regionData, src, primary);

  vector<int8_t> secondaryCodes = {StrUtf8::kDefaultCode, StrUtf8::kInternationalCode};

  vector<int8_t> mwmLangCodes;
  regionData.GetLanguages(mwmLangCodes);
  secondaryCodes.insert(secondaryCodes.end(), mwmLangCodes.begin(), mwmLangCodes.end());

  secondaryCodes.push_back(StrUtf8::kEnglishCode);

  GetBestName(src, secondaryCodes, secondary);

  if (primary.empty())
    primary.swap(secondary);
  else if (!secondary.empty() && primary.find(secondary) != string::npos)
    secondary.clear();
}

void GetReadableName(RegionData const & regionData, StringUtf8Multilang const & src,
                     int8_t const deviceLang, bool allowTranslit, string & out)
{
  out.clear();

  if (src.IsEmpty())
    return;

  // If MWM contains user's language.
  bool const preferDefault = IsNativeLang(regionData, deviceLang);

  GetReadableNameImpl(regionData, src, deviceLang, preferDefault, allowTranslit, out);
}

int8_t GetNameForSearchOnBooking(RegionData const & regionData, StringUtf8Multilang const & src,
                                 string & name)
{
  if (src.GetString(StringUtf8Multilang::kDefaultCode, name))
    return StringUtf8Multilang::kDefaultCode;

  vector<int8_t> mwmLangs;
  regionData.GetLanguages(mwmLangs);

  for (auto mwmLang : mwmLangs)
  {
    if (src.GetString(mwmLang, name))
      return mwmLang;
  }

  if (src.GetString(StringUtf8Multilang::kEnglishCode, name))
    return StringUtf8Multilang::kEnglishCode;

  name.clear();
  return StringUtf8Multilang::kUnsupportedLanguageCode;
}

bool GetPreferredName(StringUtf8Multilang const & src, int8_t deviceLang, string & out)
{
  auto const priorityList = MakePrimaryNamePriorityList(deviceLang, true /* preferDefault */);
  return GetBestName(src, priorityList, out);
}

vector<int8_t> GetDescriptionLangPriority(RegionData const & regionData, int8_t const deviceLang)
{
  bool const preferDefault = IsNativeLang(regionData, deviceLang);
  return MakePrimaryNamePriorityList(deviceLang, preferDefault);
}
} // namespace feature
