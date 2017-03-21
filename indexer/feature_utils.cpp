#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/base.hpp"

#include "3party/icu/i18n/unicode/translit.h"
#include "3party/icu/i18n/unicode/utrans.h"
#include "3party/icu/common/unicode/utypes.h"
#include "3party/icu/common/unicode/unistr.h"

#include "std/vector.hpp"

namespace
{
using StrUtf8 = StringUtf8Multilang;

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

void GetTransliteratedName(feature::RegionData const & regionData, StringUtf8Multilang const & src, string & out)
{
  vector<int8_t> mwmLangCodes;
  regionData.GetLanguages(mwmLangCodes);

  string srcName;
  for (auto const code : mwmLangCodes)
  {
    if (src.GetString(code, srcName))
    {
      out = Transliterate(srcName, StringUtf8Multilang::GetLangEnNameByCode(code));
      if (!out.empty())
        return;
    }
  }
}

void GetBestName(StringUtf8Multilang const & src, vector<int8_t> const & priorityList, string & out)
{
  auto bestIndex = priorityList.size();

  auto const findAndSet = [](vector<int8_t> const & langs, int8_t const code, string const & name,
                               size_t & bestIndex, string & outName)
  {
    auto const it = find(langs.begin(), langs.end(), code);
    if (it != langs.end() && bestIndex > distance(langs.begin(), it))
    {
      bestIndex = distance(langs.begin(), it);
      outName = name;
    }
  };

  src.ForEach([&](int8_t code, string const & name)
  {
    if (bestIndex == 0)
      return false;

    findAndSet(priorityList, code, name, bestIndex, out);

    return true;
  });

  // There are many "junk" names in Arabian island.
  if (bestIndex < priorityList.size() &&
      priorityList[bestIndex] == StrUtf8::kInternationalCode)
  {
    out = out.substr(0, out.find_first_of(','));
  }
}
}  // namespace

void initICU(std::string const & icuDataDir)
{
  u_setDataDirectory(icuDataDir.c_str());
}

std::string Transliterate(std::string const & str, std::string const & lang)
{
  UnicodeString ustr(str.c_str());
  UErrorCode status = U_ZERO_ERROR;

  const std::string id = lang + "-Latin/BGN";
  std::unique_ptr<Transliterator> latinTransliterator(Transliterator::createInstance(id.c_str(), UTRANS_FORWARD, status));
  if (latinTransliterator == nullptr)
  {
    LOG(LWARNING, ("Cannot create transliterator", id));
    return "";
  }

  latinTransliterator->transliterate(ustr);

  std::string resultStr;
  ustr.toUTF8String(resultStr);

  LOG(LDEBUG, ("Transliterated", str, "->", resultStr, "id =", id));

  return resultStr;
}

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
                       int8_t const deviceLang, string & primary, string & secondary)
{
  primary.clear();
  secondary.clear();

  if (src.IsEmpty())
    return;

  GetBestName(src, {deviceLang, StrUtf8::kInternationalCode}, primary);
  if (primary.empty())
  {
    GetTransliteratedName(regionData, src, primary);
    if (primary.empty())
      GetBestName(src, {StrUtf8::kEnglishCode}, primary);
  }

  vector<int8_t> mwmLangCodes;
  regionData.GetLanguages(mwmLangCodes);
  vector<int8_t> secondaryCodes = {StrUtf8::kDefaultCode,
                                   StrUtf8::kInternationalCode};
  secondaryCodes.insert(secondaryCodes.end(), mwmLangCodes.begin(), mwmLangCodes.end());
  secondaryCodes.push_back(StrUtf8::kEnglishCode);

  GetBestName(src, secondaryCodes, secondary);

  if (primary.empty())
    primary.swap(secondary);  
  else if (!secondary.empty() && primary.find(secondary) != string::npos)
    secondary.clear();
}

void GetReadableName(RegionData const & regionData, StringUtf8Multilang const & src,
                     int8_t const deviceLang, string & out)
{
  out.clear();

  if (src.IsEmpty())
    return;

  vector<int8_t> codes;
  // If MWM contains user's language.
  bool const preferDefault = regionData.HasLanguage(deviceLang);
  if (preferDefault)
    codes = {deviceLang, StrUtf8::kDefaultCode, StrUtf8::kInternationalCode};
  else
    codes = {deviceLang, StrUtf8::kInternationalCode};

  GetBestName(src, codes, out);
  if (out.empty())
  {
    GetTransliteratedName(regionData, src, out);
    if (out.empty())
    {
      if (preferDefault)
        codes = {StrUtf8::kEnglishCode};
      else
        codes = {StrUtf8::kEnglishCode, StrUtf8::kDefaultCode};
      GetBestName(src, codes, out);
      if (out.empty())
        GetMwmLangName(regionData, src, out);
    }
  }
}

} // namespace feature
