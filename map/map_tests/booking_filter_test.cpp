#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "map/booking_availability_filter.hpp"
#include "map/booking_filter_processor.hpp"
#include "map/booking_utils.hpp"

#include "search/result.hpp"

#include "partners_api/booking_api.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_meta.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

using namespace booking::filter;
using namespace generator::tests_support;

namespace
{
class TestMwmEnvironment : public TestWithCustomMwms,
                           public FilterBase::Delegate
{
public:
  DataSource const & GetDataSource() const override { return m_dataSource; }

  booking::Api const & GetApi() const override { return m_api; }

protected:
  TestMwmEnvironment()
  {
    booking::SetBookingUrlForTesting("http://localhost:34568/booking");
  }

  ~TestMwmEnvironment()
  {
    booking::SetBookingUrlForTesting("");
  }

  void OnMwmBuilt(MwmInfo const & info) override
  {
    m_infoGetter.AddCountry(storage::CountryDef(info.GetCountryName(), info.m_bordersRect));
  }

private:
  storage::CountryInfoGetterForTesting m_infoGetter;
  Platform::ThreadRunner m_runner;
  booking::Api m_api;
};

class TestBookingHotel : public TestPOI
{
public:
  TestBookingHotel(m2::PointD const & center, std::string const & name, std::string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"tourism", "hotel"}, {"sponsored", "booking"}});
  }

  TestBookingHotel(m2::PointD const & center, std::string const & name)
    : TestBookingHotel(center, name, "en")
  {
  }
};

void InsertResult(search::Result r, search::Results & dst)
{
  dst.AddResult(std::move(r));
}

UNIT_CLASS_TEST(TestMwmEnvironment, BookingFilter_AvailabilitySmoke)
{
  AvailabilityFilter filter(*this);

  std::vector<std::string> const kHotelIds = {"10623", "10624", "10625"};

  BuildCountry("TestMwm", [&kHotelIds](TestMwmBuilder & builder)
  {
    TestBookingHotel hotel1(m2::PointD(1.0, 1.0), "hotel 1");
    hotel1.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[0]);
    builder.Add(hotel1);

    TestBookingHotel hotel2(m2::PointD(1.1, 1.1), "hotel 2");
    hotel2.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[1]);
    builder.Add(hotel2);

    TestBookingHotel hotel3(m2::PointD(0.9, 0.9), "hotel 3");
    hotel3.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[2]);
    builder.Add(hotel3);
  });

  m2::RectD const rect(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
  search::Results results;
  results.AddResult({"suggest for testing", "suggest for testing"});
  search::Results expectedResults;
  m_dataSource.ForEachInRect(
      [&results, &expectedResults](FeatureType & ft) {
        search::Result::Details details;
        details.m_isSponsoredHotel = true;
        search::Result result(ft.GetID(), ft.GetCenter(), "", "", 0, details);
        auto copy = result;
        results.AddResult(std::move(result));
        expectedResults.AddResult(std::move(copy));
      },
      rect, scales::GetUpperScale());
  ParamsInternal filterParams;
  search::Results filteredResults;
  std::vector<booking::Extras> availabilityExtras;
  filterParams.m_apiParams = std::make_shared<booking::AvailabilityParams>(booking::AvailabilityParams::MakeDefault());
  filterParams.m_callback = [&filteredResults, &availabilityExtras](auto && result) {
    filteredResults = result.m_passedFilter;
    availabilityExtras = result.m_extras;
    testing::Notify();
  };

  filter.ApplyFilter(results, filterParams);

  testing::Wait();

  TEST_NOT_EQUAL(filteredResults.GetCount(), 0, ());
  TEST_EQUAL(filteredResults.GetCount(), expectedResults.GetCount(), ());
  TEST(!availabilityExtras.empty(), ());
  TEST_EQUAL(availabilityExtras.size(), filteredResults.GetCount(), ());

  for (size_t i = 0; i < filteredResults.GetCount(); ++i)
  {
    TEST_EQUAL(filteredResults[i].GetFeatureID(), expectedResults[i].GetFeatureID(), ());
  }
}

UNIT_CLASS_TEST(TestMwmEnvironment, BookingFilter_ProcessorSmoke)
{
  std::vector<std::string> const kHotelIds = {"10622", "10623", "10624", "10625", "10626"};
  std::set<std::string> const kAvailableHotels = {"10623", "10624", "10625"};
  std::set<std::string> const kHotelsWithDeals = {"10622", "10624", "10626"};

  BuildCountry("TestMwm", [&kHotelIds](TestMwmBuilder & builder)
  {
    TestBookingHotel hotel1(m2::PointD(0.7, 0.7), "hotel 22", "en");
    hotel1.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[0]);
    builder.Add(hotel1);

    TestBookingHotel hotel2(m2::PointD(0.8, 0.8), "hotel 23", "en");
    hotel2.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[1]);
    builder.Add(hotel2);

    TestBookingHotel hotel3(m2::PointD(0.9, 0.9), "hotel 24", "en");
    hotel3.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[2]);
    builder.Add(hotel3);

    TestBookingHotel hotel4(m2::PointD(1.0, 1.0), "hotel 25", "en");
    hotel4.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[3]);
    builder.Add(hotel4);

    TestBookingHotel hotel5(m2::PointD(1.1, 1.1), "hotel 26", "en");
    hotel5.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[4]);
    builder.Add(hotel5);
  });

  std::set<std::string> availableWithDeals;
  for (auto const & available : kAvailableHotels)
  {
    if (kHotelsWithDeals.find(available) != kHotelsWithDeals.cend())
      availableWithDeals.insert(available);
  }

  m2::RectD const rect(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
  search::Results results;
  results.AddResult({"suggest for testing", "suggest for testing"});
  search::Results expectedAvailabilityResults;
  search::Results expectedDealsResults;
  search::Results expectedAvailableWithDeals;
  m_dataSource.ForEachInRect(
    [&](FeatureType & ft) {
      search::Result::Details details;
      details.m_isSponsoredHotel = true;
      std::string name;
      ft.GetName(StringUtf8Multilang::kDefaultCode, name);
      search::Result result(ft.GetID(), ft.GetCenter(), name, "", 0, details);
      InsertResult(result, results);

      auto const sponsoredId = ft.GetMetadata(feature::Metadata::FMD_SPONSORED_ID);

      if (kAvailableHotels.find(sponsoredId) != kAvailableHotels.cend())
        InsertResult(result, expectedAvailabilityResults);

      if (kHotelsWithDeals.find(sponsoredId) != kHotelsWithDeals.cend())
        InsertResult(result, expectedDealsResults);

      if (availableWithDeals.find(sponsoredId) != availableWithDeals.cend())
        InsertResult(result, expectedAvailableWithDeals);
    },
    rect, scales::GetUpperScale());

  TasksInternal tasks;
  ParamsInternal availabilityParams;
  search::Results availabilityResults;
  std::vector<booking::Extras> availabilityExtras;
  availabilityParams.m_apiParams = std::make_shared<booking::AvailabilityParams>(booking::AvailabilityParams::MakeDefault());
  availabilityParams.m_callback = [&availabilityResults, &availabilityExtras](auto && result) {
    availabilityResults = result.m_passedFilter;
    availabilityExtras = result.m_extras;
  };

  tasks.emplace_back(Type::Availability, std::move(availabilityParams));

  ParamsInternal dealsParams;
  search::Results dealsResults;
  std::vector<booking::Extras> dealsExtras;
  booking::AvailabilityParams p = booking::AvailabilityParams::MakeDefault();
  p.m_dealsOnly = true;
  dealsParams.m_apiParams = std::make_shared<booking::AvailabilityParams>(p);
  dealsParams.m_callback = [&dealsResults, &dealsExtras](auto && result) {
    dealsResults = result.m_passedFilter;
    dealsExtras = result.m_extras;
    testing::Notify();
  };

  tasks.emplace_back(Type::Deals, std::move(dealsParams));

  FilterProcessor processor(GetDataSource(), GetApi());
  auto tasksCopy = tasks;
  processor.ApplyFilters(results, std::move(tasksCopy), ApplicationMode::Independent);

  testing::Wait();

  TEST_NOT_EQUAL(availabilityResults.GetCount(), 0, ());
  TEST_EQUAL(availabilityResults.GetCount(), expectedAvailabilityResults.GetCount(), ());
  TEST(!availabilityExtras.empty(), ());
  TEST_EQUAL(availabilityExtras.size(), availabilityResults.GetCount(), ());

  for (size_t i = 0; i < availabilityResults.GetCount(); ++i)
  {
    TEST_EQUAL(availabilityResults[i].GetFeatureID(),
               expectedAvailabilityResults[i].GetFeatureID(), ());
  }

  TEST_NOT_EQUAL(dealsResults.GetCount(), 0, ());
  TEST_EQUAL(dealsResults.GetCount(), expectedDealsResults.GetCount(), ());
  TEST(!dealsExtras.empty(), ());
  TEST_EQUAL(dealsExtras.size(), dealsResults.GetCount(), ());

  for (size_t i = 0; i < dealsResults.GetCount(); ++i)
  {
    TEST_EQUAL(dealsResults[i].GetFeatureID(), expectedDealsResults[i].GetFeatureID(), ());
  }

  processor.ApplyFilters(results, std::move(tasks), ApplicationMode::Consecutive);

  testing::Wait();

  TEST_NOT_EQUAL(availabilityResults.GetCount(), 0, ());
  TEST_EQUAL(availabilityResults.GetCount(), expectedAvailabilityResults.GetCount(), ());

  for (size_t i = 0; i < availabilityResults.GetCount(); ++i)
  {
    TEST_EQUAL(availabilityResults[i].GetFeatureID(),
               expectedAvailabilityResults[i].GetFeatureID(), ());
  }

  TEST_NOT_EQUAL(dealsResults.GetCount(), 0, ());
  TEST_EQUAL(dealsResults.GetCount(), expectedAvailableWithDeals.GetCount(), ());

  for (size_t i = 0; i < dealsResults.GetCount(); ++i)
  {
    TEST_EQUAL(dealsResults[i].GetFeatureID(), expectedAvailableWithDeals[i].GetFeatureID(), ());
  }
}

UNIT_CLASS_TEST(TestMwmEnvironment, BookingFilter_ApplyFilterOntoWithFeatureIds)
{
  AvailabilityFilter filter(*this);

  std::vector<std::string> const kHotelIds = {"10623", "10624", "10625"};

  BuildCountry("TestMwm", [&kHotelIds](TestMwmBuilder & builder)
  {
    TestBookingHotel hotel1(m2::PointD(1.0, 1.0), "hotel 1");

    hotel1.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[0]);
    builder.Add(hotel1);

    TestBookingHotel hotel2(m2::PointD(1.1, 1.1), "hotel 2");
    hotel2.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[1]);
    builder.Add(hotel2);

    TestBookingHotel notExpectedHotel(m2::PointD(0.9, 0.9), "hotel 900");
    notExpectedHotel.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, "123456");
    builder.Add(notExpectedHotel);

    TestBookingHotel hotel3(m2::PointD(0.9, 0.9), "hotel 3");
    hotel3.GetMetadata().Set(feature::Metadata::FMD_SPONSORED_ID, kHotelIds[2]);
    builder.Add(hotel3);

    TestPOI notExpectedFeature(m2::PointD(0.8, 0.8), "some feature", "en");
    builder.Add(notExpectedFeature);
  });

  m2::RectD const rect(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
  search::Results results;
  results.AddResult({"suggest for testing", "suggest for testing"});
  std::vector<FeatureID> allFeatureIds;
  std::vector<FeatureID> expectedFeatureIds;
  m_dataSource.ForEachInRect(
    [&results, &allFeatureIds, &expectedFeatureIds, &kHotelIds](FeatureType & ft) {
      search::Result::Details details;
      details.m_isSponsoredHotel = true;
      search::Result result(ft.GetID(), ft.GetCenter(), "", "", 0, details);
      auto copy = result;
      results.AddResult(std::move(result));

      allFeatureIds.push_back(ft.GetID());

      auto hotelId = ft.GetMetadata(feature::Metadata::FMD_SPONSORED_ID);
      if (kHotelIds.cend() != std::find(kHotelIds.cbegin(), kHotelIds.cend(), hotelId))
        expectedFeatureIds.push_back(ft.GetID());
    },
    rect, scales::GetUpperScale());

  std::sort(expectedFeatureIds.begin(), expectedFeatureIds.end());
  std::sort(allFeatureIds.begin(), allFeatureIds.end());

  ParamsRawInternal filterParams;
  std::vector<FeatureID> filteredResults;
  std::vector<booking::Extras> availabilityExtras;
  filterParams.m_apiParams = std::make_shared<booking::AvailabilityParams>(booking::AvailabilityParams::MakeDefault());
  filterParams.m_callback = [&filteredResults, &availabilityExtras](auto && result) {
    filteredResults = result.m_passedFilter;
    availabilityExtras = result.m_extras;
    testing::Notify();
  };

  filter.ApplyFilter(allFeatureIds, filterParams);

  testing::Wait();

  TEST_NOT_EQUAL(filteredResults.size(), 0, ());
  TEST_EQUAL(filteredResults.size(), expectedFeatureIds.size(), ());
  TEST_EQUAL(filteredResults, expectedFeatureIds, ());
  TEST(!availabilityExtras.empty(), ());
  TEST_EQUAL(availabilityExtras.size(), filteredResults.size(), ());
}

UNIT_TEST(Booking_PriceFormatter)
{
  booking::PriceFormatter formatter;

  {
    auto const result = formatter.Format(1, "USD");
    TEST(strings::StartsWith(result, "1 "), ());
  }
  {
    auto const result = formatter.Format(12, "USD");
    TEST(strings::StartsWith(result, "12 "), ());
  }
  {
    auto const result = formatter.Format(123, "USD");
    TEST(strings::StartsWith(result, "123 "), ());
  }
  {
    auto const result = formatter.Format(1234, "USD");
    TEST(strings::StartsWith(result, "1,234 "), (result));
  }
  {
    auto const result = formatter.Format(12345, "USD");
    TEST(strings::StartsWith(result, "12,345 "), (result));
  }
  {
    auto const result = formatter.Format(123456, "USD");
    TEST(strings::StartsWith(result, "123,456 "), (result));
  }
  {
    auto const result = formatter.Format(1234567, "USD");
    TEST(strings::StartsWith(result, "1,234,567 "), (result));
  }
  {
    auto const result = formatter.Format(12345678, "USD");
    TEST(strings::StartsWith(result, "12,345,678 "), (result));
  }
  {
    auto const result = formatter.Format(123456789, "USD");
    TEST(strings::StartsWith(result, std::string("123,456,") + u8"\u2026" + " "), (result));
  }
  {
    auto const result = formatter.Format(1234567891, "USD");
    TEST(strings::StartsWith(result, std::string("1,234,56") + u8"\u2026" + " "), (result));
  }
  {
    auto const result = formatter.Format(12345678911, "USD");
    TEST(strings::StartsWith(result, std::string("12,345,6") + u8"\u2026" + " "), (result));
  }
}
}  // namespace
