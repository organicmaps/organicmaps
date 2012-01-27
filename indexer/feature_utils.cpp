#include "feature_utils.hpp"
#include "feature_visibility.hpp"
#include "classificator.hpp"
#include "feature.hpp"
#include "scales.hpp"

#include "../geometry/point2d.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"


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

  void CorrectScaleForVisibility(FeatureType const & f, int & scale) const
  {
    pair<int, int> const scaleR = feature::DrawableScaleRangeForText(f);
    if (scale < scaleR.first || scale > scaleR.second)
      scale = scaleR.first;
  }

  void CorrectRectForScales(FeatureType const & f, m2::PointD const & center, m2::RectD & rect) const
  {
    int const scale = scales::GetScaleLevel(rect);
    int scaleNew = scale;
    CorrectScaleForVisibility(f, scaleNew);

    if (scale != scaleNew)
      rect = scales::GetRectForLevel(scaleNew, center, 1.0);
  }

  m2::RectD GetViewport(FeatureType const & f) const
  {
    m2::RectD limitR = f.GetLimitRect(-2);
    if (f.GetFeatureType() != feature::GEOM_POINT)
    {
      CorrectRectForScales(f, limitR.Center(), limitR);
      return limitR;
    }

    feature::TypesHolder types(f);

    int const upperScale = scales::GetUpperScale();
    int scale = upperScale;
    for (size_t i = 0; i < types.Size(); ++i)
      scale = min(scale, GetScaleForType(types[i], f));

    CorrectScaleForVisibility(f, scale);

    m2::PointD const centerXY = limitR.Center();
    return scales::GetRectForLevel(scale, centerXY, 1.0);
  }

  uint8_t GetSearchRank(FeatureType const & feature) const
  {
    uint32_t population = feature.GetPopulation();

    feature::TypesHolder types(feature);

    for (size_t i = 0; i < types.Size(); ++i)
    {
      if (IsEqual(types[i], m_TypeSmallVillage))
        population = max(population, static_cast<uint32_t>(100));
      else if (IsEqual(types[i], m_TypeVillage))
        population = max(population, static_cast<uint32_t>(1000));
      else if (types[i] == m_TypeTown || IsEqual(types[i], m_TypeCounty))
        population = max(population, static_cast<uint32_t>(10000));
      else if (types[i] == m_TypeCity || types[i] == m_TypeState)
        population = max(population, static_cast<uint32_t>(100000));
      else if (types[i] == m_TypeCityCapital)
        population = max(population, static_cast<uint32_t>(1000000));
      else if (types[i] == m_TypeCountry)
        population = max(population, static_cast<uint32_t>(2000000));
      else if (types[i] == m_TypeContinent)
        population = max(population, static_cast<uint32_t>(20000000));
    }

    return static_cast<uint8_t>(log(double(population)) / log(1.1));
  }

private:

  // Returns width and height (lon and lat) for a given type.
  int GetScaleForType(uint32_t const type, FeatureType const & feature) const
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

    return scales::GetUpperScale();
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

m2::RectD GetFeatureViewport(FeatureType const & feature)
{
  return impl::GetFeatureEstimator().GetViewport(feature);
}

uint8_t GetSearchRank(FeatureType const & feature)
{
  return impl::GetFeatureEstimator().GetSearchRank(feature);
}

} // namespace feature
