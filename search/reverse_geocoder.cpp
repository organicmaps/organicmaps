#include "reverse_geocoder.hpp"
#include "search_string_utils.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"


namespace search
{

namespace
{

double constexpr kLookupRadiusM = 500.0;
size_t constexpr kMaxStreetIndex = 16;
size_t constexpr kSimilarityThresholdPercent = 10;


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
void ReverseGeocoder::GetNearbyStreets(FeatureType const & addrFt, TCompare comp,
                                        vector<Street> & streets)
{
  m2::PointD const & center = feature::GetCenter(addrFt);
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        center, kLookupRadiusM);

  auto const fn = [&](FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE)
      return;

    if (!ftypes::IsStreetChecker::Instance()(ft))
      return;

    string name;
    static int8_t const lang = StringUtf8Multilang::GetLangIndex("default");
    if (!ft.GetName(lang, name))
      return;

    ASSERT(!name.empty(), ());
    streets.push_back({ft.GetID(), CalculateMinDistance(ft, center), comp(name)});
  };

  m_index.ForEachInRect(fn, rect, scales::GetUpperScale());

  sort(streets.begin(), streets.end(), [](Street const & s1, Street const & s2)
  {
    return s1.m_distance < s2.m_distance;
  });
}

void ReverseGeocoder::GetNearbyStreets(FeatureType const & addrFt, string const & keyName,
                                        vector<Street> & streets)
{
  strings::UniString const uniKey1 = strings::MakeUniString(keyName);

  GetNearbyStreets(addrFt, [&uniKey1](string const & name) -> pair<size_t, size_t>
  {
    string key;
    search::GetStreetNameAsKey(name, key);
    strings::UniString const uniKey2 = strings::MakeUniString(key);
    /// @todo Transforming street name into street name key may produce empty strings.
    //ASSERT(!uniKey2.empty(), ());

    return { strings::EditDistance(uniKey1.begin(), uniKey1.end(), uniKey2.begin(), uniKey2.end()),
             uniKey2.size() };
  }, streets);
}

size_t ReverseGeocoder::GetMatchedStreetIndex(vector<Street> const & streets)
{
  // Do limit possible return values.
  size_t const count = min(streets.size(), kMaxStreetIndex);

  // Find the exact match or the best match in kSimilarityTresholdPercent limit.
  size_t res = count;
  size_t minPercent = kSimilarityThresholdPercent + 1;
  for (size_t i = 0; i < count; ++i)
  {
    if (streets[i].m_editDistance.first == 0)
      return i;
    if (streets[i].m_editDistance.second == 0)
      continue;

    size_t const p = streets[i].m_editDistance.first * 100 / streets[i].m_editDistance.second;
    if (p < minPercent)
    {
      res = i;
      minPercent = p;
    }
  }

  return (res < count ? res : streets.size());
}

} // namespace search
