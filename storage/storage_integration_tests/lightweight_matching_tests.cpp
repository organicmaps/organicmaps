#include "testing/testing.hpp"

#include "storage/country_info_reader_light.hpp"
#include "storage/index.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <utility>
#include <vector>

using namespace lightweight;

namespace
{
double constexpr kStepInMercator = 1;  // 1 mercator ~= 9602.84 meters

struct PointAndCountry
{
  PointAndCountry(m2::PointD && pt, storage::TCountryId && country)
    : m_pt(std::move(pt))
    , m_country(std::move(country))
  {
  }

  m2::PointD m_pt;
  storage::TCountryId m_country;
};

UNIT_CLASS_TEST(CountryInfoReader, LightweightMatching)
{
  auto const reader = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());

  LOG(LINFO, ("Generating dataset..."));
  std::vector<PointAndCountry> dataset;
  for (auto x = MercatorBounds::minX; x <= MercatorBounds::maxX; x += kStepInMercator)
  {
    for (auto y = MercatorBounds::minY; y <= MercatorBounds::maxY; y += kStepInMercator)
    {
      m2::PointD pt(x, y);
      dataset.emplace_back(std::move(pt), reader->GetRegionCountryId(pt));
    }
  }

  {
    m2::PointD ptFrom = {MercatorBounds::minX, MercatorBounds::minY};
    m2::PointD ptTo = {MercatorBounds::minX + kStepInMercator, MercatorBounds::minY};
    auto const stepSizeInMeters = MercatorBounds::DistanceOnEarth(ptFrom, ptTo);
    LOG(LINFO, ("The dataset is generated. Dataset size:", dataset.size(),
                ". The step is:", stepSizeInMeters, "meters"));
  }

  for (auto const & sample : dataset)
  {
    TEST_EQUAL(GetRegionCountryId(sample.m_pt), sample.m_country, (sample.m_pt));
  }
}
}  // namespace
