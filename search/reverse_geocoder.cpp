#include "reverse_geocoder.hpp"
#include "search_string_utils.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/stl_helpers.hpp"

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

void ReverseGeocoder::GetNearbyStreets(FeatureType const & addrFt, vector<Street> & streets)
{
  m2::PointD const & center = feature::GetCenter(addrFt);
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        center, kLookupRadiusM);

  auto const addStreet = [&](FeatureType const & ft)
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
    streets.push_back({ft.GetID(), CalculateMinDistance(ft, center), name});
  };

  m_index.ForEachInRect(addStreet, rect, scales::GetUpperScale());
  sort(streets.begin(), streets.end(), my::CompareBy(&Street::m_distanceMeters));
}

// static
size_t ReverseGeocoder::GetMatchedStreetIndex(string const & keyName,
                                              vector<Street> const & streets)
{
  strings::UniString const expected = strings::MakeUniString(keyName);

  // Do limit possible return values.
  size_t const count = min(streets.size(), kMaxStreetIndex);

  // Find the exact match or the best match in kSimilarityTresholdPercent limit.
  size_t result = count;
  size_t minPercent = kSimilarityThresholdPercent + 1;
  for (size_t i = 0; i < count; ++i)
  {
    string key;
    search::GetStreetNameAsKey(streets[i].m_name, key);
    strings::UniString const actual = strings::MakeUniString(key);

    size_t const editDistance =
        strings::EditDistance(expected.begin(), expected.end(), actual.begin(), actual.end());

    if (editDistance == 0)
      return i;

    if (actual.empty())
      continue;

    size_t const percent = editDistance * 100 / actual.size();
    if (percent < minPercent)
    {
      result = i;
      minPercent = percent;
    }
  }

  return (result < count ? result : streets.size());
}
} // namespace search
