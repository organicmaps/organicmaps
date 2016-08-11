#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/base.hpp"

#include "std/vector.hpp"

namespace
{
void GetMwmLangName(FeatureID const & id, StringUtf8Multilang const & src, string & out)
{
  auto const mwmInfo = id.m_mwmId.GetInfo();

  if (!mwmInfo)
    return;

  vector<int8_t> mwmLangCodes;
  mwmInfo->GetRegionData().GetLanguages(mwmLangCodes);

  for (auto const code : mwmLangCodes)
  {
    if (src.GetString(code, out))
      return;
  }
}

void GetNames(FeatureID const & id, StringUtf8Multilang const & src, string & primary,
              string & secondary)
{
  vector<int8_t> primaryCodes = {StringUtf8Multilang::kDefaultCode};
  vector<int8_t> secondaryCodes = {StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm()),
                                   StringUtf8Multilang::kInternationalCode,
                                   StringUtf8Multilang::kEnglishCode};

  auto primaryIndex = primaryCodes.size();
  auto secondaryIndex = secondaryCodes.size();

  primary.clear();
  secondary.clear();

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
    if (primaryIndex != 0)
      findAndSet(primaryCodes, code, name, primaryIndex, primary);

    if (secondaryIndex != 0)
      findAndSet(secondaryCodes, code, name, secondaryIndex, secondary);

    return true;
  });

  if (primary.empty())
    GetMwmLangName(id, src, primary);

  if (secondaryIndex < secondaryCodes.size() &&
      secondaryCodes[secondaryIndex] == StringUtf8Multilang::kInternationalCode)
  {
    // There are many "junk" names in Arabian island.
    secondary = secondary.substr(0, secondary.find_first_of(','));
  }
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

public:

  FeatureEstimator()
  {
    m_TypeContinent   = GetType("place", "continent");
    m_TypeCountry     = GetType("place", "country");

    m_TypeState       = GetType("place", "state");
    m_TypeCounty[0]   = GetType("place", "region");
    m_TypeCounty[1]   = GetType("place", "county");

    m_TypeCity        = GetType("place", "city");
    m_TypeCityCapital = GetType("place", "city", "capital");
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

    if (type == m_TypeCity || type == m_TypeCityCapital)
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
  uint32_t m_TypeCityCapital;
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

void GetPreferredNames(FeatureID const & id, StringUtf8Multilang const & src, string & primary,
                       string & secondary)
{
  // Primary name using priority:
  // - default name;
  // - country language name.
  // Secondary name using priority:
  // - device language name;
  // - international name;
  // - english name.
  GetNames(id, src, primary, secondary);

  if (primary.empty())
  {
    primary.swap(secondary);
  }
  else
  {
    // Filter out similar intName.
    if (!secondary.empty() && primary.find(secondary) != string::npos)
      secondary.clear();
  }
}

void GetReadableName(FeatureID const & id, StringUtf8Multilang const & src, string & out)
{
  // Names using priority:
  // - device language name;
  // - international name;
  // - english name;
  // - default name;
  // - country language name.
  // Secondary name is preffered to display on the map and place page.
  string primary, secondary;
  GetNames(id, src, primary, secondary);
  out = secondary.empty() ? primary : secondary;
}
} // namespace feature
