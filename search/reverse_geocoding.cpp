#include "reverse_geocoding.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"


namespace search
{

namespace
{

double constexpr kLookupRadiusM = 500.0;
size_t const kMaxStreetIndex = 16;
size_t const kPossiblePercent = 10;


/// @todo Need to check projection here?
double CalculateMinDistance(FeatureType const & ft, m2::PointD const & pt)
{
  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());

  double res = numeric_limits<double>::max();
  ft.ForEachPoint([&] (m2::PointD const & p)
  {
    double const d = MercatorBounds::DistanceOnEarth(p, pt);
    if (d < res)
      res = d;
  }, FeatureType::BEST_GEOMETRY);

  return res;
}

} // namespace

template <class TCompare>
void ReverseGeocoding::GetNearbyStreets(FeatureType const & addrFt, TCompare comp,
                                        vector<Street> & streets)
{
  m2::PointD const & center = feature::GetCenter(addrFt);
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        center, kLookupRadiusM);

  auto const fn = [&](FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE)
      return;

    static feature::TypeSetChecker checker({"highway"});
    feature::TypesHolder types(ft);
    if (!checker.IsEqualR(types.begin(), types.end()))
      return;

    string name;
    static int8_t const lang = StringUtf8Multilang::GetLangIndex("default");
    if (!ft.GetName(lang, name))
      return;

    ASSERT(!name.empty(), ());
    streets.push_back({ft.GetID(), CalculateMinDistance(ft, center), comp(name)});
  };

  m_index->ForEachInRect(fn, rect, scales::GetUpperScale());

  sort(streets.begin(), streets.end(), [](Street const & s1, Street const & s2)
  {
    return s1.m_distance < s2.m_distance;
  });
}

void ReverseGeocoding::GetNearbyStreets(FeatureType const & addrFt, string const & keyName,
                                        vector<Street> & streets)
{
  strings::UniString const uniKey1 = strings::MakeUniString(keyName);

  GetNearbyStreets(addrFt, [&uniKey1](string const & name) -> pair<size_t, size_t>
  {
    string key;
    search::GetStreetNameAsKey(name, key);
    strings::UniString const uniKey2 = strings::MakeUniString(key);

    return { strings::EditDistance(uniKey1.begin(), uniKey1.end(), uniKey2.begin(), uniKey2.end()),
             uniKey1.size() };
  }, streets);
}

size_t ReverseGeocoding::GetMatchedStreetIndex(vector<Street> const & streets)
{
  // do limit possible return values
  size_t const count = min(streets.size(), kMaxStreetIndex);

  // try to find exact match
  for (size_t i = 0; i < count; ++i)
    if (streets[i].m_editDistance.first == 0)
      return i;

  // try to find best match in kPossiblePercent limit
  size_t res = count;
  size_t minPercent = kPossiblePercent + 1;
  for (size_t i = 0; i < count; ++i)
  {
    size_t const p = streets[i].m_editDistance.first * 100 / streets[i].m_editDistance.second;
    if (p < kPossiblePercent)
    {
      res = i;
      minPercent = p;
    }
  }

  return (res < count ? res : streets.size());
}

} // namespace search
