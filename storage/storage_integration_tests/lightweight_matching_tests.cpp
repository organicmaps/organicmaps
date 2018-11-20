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
double constexpr kStepInMercator = 1;

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
  for (auto x = MercatorBounds::kMinX; x <= MercatorBounds::kMaxX; x += kStepInMercator)
  {
    for (auto y = MercatorBounds::kMinY; y <= MercatorBounds::kMaxY; y += kStepInMercator)
    {
      m2::PointD pt(x, y);
      dataset.emplace_back(std::move(pt), reader->GetRegionCountryId(pt));
    }
  }

  LOG(LINFO, ("The dataset is generated. Dataset size:", dataset.size()));

  for (auto const & sample : dataset)
  {
    TEST_EQUAL(GetRegionCountryId(sample.m_pt), sample.m_country, (sample.m_pt));
  }
}
}  // namespace
