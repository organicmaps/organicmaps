#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "indexer/indexer_tests_support/test_mwm_environment.hpp"

#include "map/booking_filter.hpp"

#include "partners_api/booking_api.hpp"

#include "search/result.hpp"

#include "indexer/feature_meta.hpp"

#include "base/scope_guard.hpp"

#include <utility>

using namespace booking::filter;
using namespace generator::tests_support;

namespace
{
UNIT_TEST(BookingFilter_AvailabilitySmoke)
{
  indexer::tests_support::TestMwmEnvironment env;
  booking::Api api;
  Filter filter(env.GetIndex(), api);

  booking::SetBookingUrlForTesting("http://localhost:34568/booking/min_price");
  MY_SCOPE_GUARD(cleanup, []() { booking::SetBookingUrlForTesting(""); });

  std::vector<std::string> const kHotelIds = {"10623", "10624", "10625"};

  env.BuildMwm("TestMwm", [&kHotelIds](TestMwmBuilder & builder)
  {
    feature::Metadata metadata;
    metadata.Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[0]);
    TestPOI hotel1(m2::PointD(1.0, 1.0), "hotel 1", "en");
    hotel1.SetMetadata(metadata);
    builder.Add(hotel1);

    TestPOI hotel2(m2::PointD(1.1, 1.1), "hotel 2", "en");
    metadata.Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[1]);
    hotel2.SetMetadata(metadata);
    builder.Add(hotel2);

    TestPOI hotel3(m2::PointD(0.9, 0.9), "hotel 3", "en");
    metadata.Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[2]);
    hotel3.SetMetadata(metadata);
    builder.Add(hotel3);
  });

  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
      m2::PointD(1.0, 1.0), 2 / MercatorBounds::degreeInMetres /* rect width */);
  search::Results results;
  results.AddResult({"suggest for testing", "suggest for testing"});
  search::Results expctedResults;
  env.GetIndex().ForEachInRect(
      [&results, &expctedResults](FeatureType & ft) {
        search::Result::Metadata metadata;
        metadata.m_isSponsoredHotel = true;
        search::Result result(ft.GetID(), ft.GetCenter(), "", "", "", 0, metadata);
        auto copy = result;
        results.AddResult(std::move(result));
        expctedResults.AddResult(std::move(copy));
      },
      rect, scales::GetUpperScale());
  availability::ParamsInternal params;
  search::Results filteredResults;
  params.m_callback = [&filteredResults](search::Results const & results)
  {
    filteredResults = results;
    testing::Notify();
  };

  filter.Availability(results, params);

  testing::Wait();

  TEST_NOT_EQUAL(filteredResults.GetCount(), 0, ());
  TEST_EQUAL(filteredResults.GetCount(), expctedResults.GetCount(), ());

  for (size_t i = 0; i < filteredResults.GetCount(); ++i)
  {
    TEST_EQUAL(filteredResults[i].GetFeatureID(), expctedResults[i].GetFeatureID(), ());
  }
}
}  // namespace