#include "testing/testing.hpp"

#include "partners_api/promo_api.hpp"

#include "platform/settings.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include <memory>

using namespace eye;
using namespace platform::tests_support;

namespace
{
// It should be compatible with test id on test server side at tools/python/ResponseProvider.py:150.
// On server side this value is negative because of the prod server does not support unsigned values.
std::string const kTestId = "13835058055282357840";

class ScopedEyeWithAsyncGuiThread : public AsyncGuiThread
{
public:
  ScopedEyeWithAsyncGuiThread()
  {
    EyeForTesting::ResetEye();
  }

  ~ScopedEyeWithAsyncGuiThread() override
  {
    EyeForTesting::ResetEye();
  }
};

class DelegateForTesting : public promo::Api::Delegate
{
public:
  std::string GetCityId(m2::PointD const &) override { return kTestId; }
  platform::HttpClient::Headers GetHeaders() override { return {}; }
};
}  // namespace

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, Promo_GetAfterBooking)
{
  promo::Api api;
  std::string lang = "en";

  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), true, ());

  Info info;
  info.m_promo.m_transitionToBookingTime = Clock::now() - std::chrono::hours(2);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), true, ());

  info.m_promo.m_transitionToBookingTime = Clock::now() - std::chrono::minutes(6);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), false, ());
}

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, Promo_GetCityGallery)
{
  promo::Api api("http://localhost:34568/");
  api.SetDelegate(std::make_unique<DelegateForTesting>());
  auto const lang = "en";

  {
    promo::CityGallery result{};
    api.GetCityGallery({}, lang, UTM::None, [&result](promo::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    },
    []
    {
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.m_items.size(), 2, ());
  }
  {
    promo::CityGallery result{};
    m2::PointD pt;
    api.GetCityGallery(pt, lang, UTM::None, [&result](promo::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    },
    []
    {
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.m_items.size(), 2, ());
  }
}

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, Promo_GetCityGallerySingleItem)
{
  {
    promo::Api api("http://localhost:34568/single/empty/");
    api.SetDelegate(std::make_unique<DelegateForTesting>());
    auto const lang = "en";

    promo::CityGallery result{};
    api.GetCityGallery({}, lang, UTM::None, [&result](promo::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    },
    []
    {
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.m_items.size(), 0, ());
  }
  {
    promo::Api api("http://localhost:34568/single/");
    api.SetDelegate(std::make_unique<DelegateForTesting>());
    auto const lang = "en";

    promo::CityGallery result{};
    m2::PointD pt;
    api.GetCityGallery(pt, lang, UTM::None, [&result](promo::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    },
    []
    {
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.m_items.size(), 1, ());
  }
}
