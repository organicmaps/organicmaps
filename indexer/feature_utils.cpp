#include "feature_utils.hpp"
#include "classificator.hpp"
#include "feature.hpp"

#include "../geometry/point2d.hpp"
#include "../base/base.hpp"
#include "../std/vector.hpp"

namespace feature
{

namespace impl
{

class FeatureEstimator
{
public:

  FeatureEstimator()
  {
    m_TypeContinent   = GetType("place", "continent");
    m_TypeCountry     = GetType("place", "country");
    m_TypeCity        = GetType("place", "city");
    m_TypeCityCapital = GetType("place", "city", "capital");
    m_TypeTown        = GetType("place", "town");
    m_TypeVillage     = GetType("place", "village");
  }

  m2::RectD GetViewport(FeatureType const & feature) const
  {
    m2::RectD limitRect = feature.GetLimitRect(-2);
    if (feature.GetFeatureType() != feature::GEOM_POINT)
      return limitRect;

    m2::PointD maxSizeMeters(100, 100);
    FeatureBase::GetTypesFn types;
    feature.ForEachTypeRef(types);
    for (size_t i = 0; i < types.m_size; ++i)
    {
      m2::PointD const sizeMeters = GetSizeForType(types.m_types[i], feature);
      maxSizeMeters.x = max(maxSizeMeters.x, sizeMeters.x);
      maxSizeMeters.y = max(maxSizeMeters.y, sizeMeters.y);
    }

    m2::PointD const centerXY = limitRect.Center();
    return MercatorBounds::RectByCenterXYAndSizeInMeters(centerXY.x, centerXY.y,
                                                         maxSizeMeters.x, maxSizeMeters.y);
  }

  uint8_t GetSearchRank(FeatureType const & feature) const
  {
    uint32_t population = feature.GetPopulation();

    FeatureBase::GetTypesFn types;
    feature.ForEachTypeRef(types);
    for (size_t i = 0; i < types.m_size; ++i)
    {
      if (types.m_types[i] == m_TypeVillage)
        population = max(population, static_cast<uint32_t>(1000));
      else if (types.m_types[i] == m_TypeTown)
        population = max(population, static_cast<uint32_t>(10000));
      else if (types.m_types[i] == m_TypeCity)
        population = max(population, static_cast<uint32_t>(100000));
      else if (types.m_types[i] == m_TypeCityCapital)
        population = max(population, static_cast<uint32_t>(1000000));
      else if (types.m_types[i] == m_TypeCountry)
        population = max(population, static_cast<uint32_t>(2000000));
      else if (types.m_types[i] == m_TypeContinent)
        population = max(population, static_cast<uint32_t>(20000000));
    }

    return static_cast<uint8_t>(log(double(population)) / log(1.1));
  }

private:

  // Returns width and height (lon and lat) for a given type.
  m2::PointD GetSizeForType(uint32_t const type, FeatureType const & feature) const
  {
    static double const km = 1000.0;
    if (type == m_TypeContinent)
      return m2::PointD(5000*km, 5000*km);

    /// @todo Load countries bounding rects.
    if (type == m_TypeCountry)
      return m2::PointD(500*km, 500*km);

    if (type == m_TypeCity || type == m_TypeCityCapital)
    {
      double const radius = sqrt(static_cast<double>(feature.GetPopulation() / 3000));
      return m2::PointD(radius*km, radius*km);
    }

    if (type == m_TypeTown)
      return m2::PointD(8*km, 8*km);
    if (type == m_TypeVillage)
      return m2::PointD(3*km, 3*km);
    return m2::PointD(0, 0);
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
  uint32_t m_TypeCity;
  uint32_t m_TypeCityCapital;
  uint32_t m_TypeTown;
  uint32_t m_TypeVillage;
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
