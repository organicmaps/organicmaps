#include "testing/testing.hpp"

#include "partners_api/promo_api.hpp"

#include "platform/settings.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include <algorithm>
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
};
}  // namespace

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, Promo_NeedToShowAfterBooking)
{
  promo::Api api;
  Info info;
  std::string lang = "en";
  {
    MapObject poi;
    poi.SetBestType("tourism-hotel");
    poi.SetPos({53.652007, 108.143443});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours((24 * 30 * 3) + 1);
    eventInfo.m_userPos = {72.045507, 81.408095};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime =
        Clock::now() - (std::chrono::hours(24 * 30 * 3) + std::chrono::seconds(1));
    eventInfo.m_userPos = {72.045400, 81.408200};
    eventInfo.m_type = MapObject::Event::Type::AddToBookmark;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 30 * 3);
    eventInfo.m_userPos = {72.045450, 81.408201};
    eventInfo.m_type = MapObject::Event::Type::RouteToCreated;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  EyeForTesting::SetInfo(info);
  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), false, ());

  {
    MapObject poi;
    poi.SetBestType("tourism-hotel");
    poi.SetPos({53.652005, 108.143448});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 30 * 3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(2);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::BookingBook;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  info.m_promo.m_transitionToBookingTime = Clock::now() - std::chrono::hours(2);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), false, ());

  {
    MapObject poi;
    poi.SetBestType("tourism-hotel");
    poi.SetPos({53.653005, 108.143548});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 20 * 3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::minutes(6);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::BookingReviews;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::minutes(3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::minutes(1);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::RouteToCreated;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  info.m_promo.m_transitionToBookingTime = Clock::now() - std::chrono::minutes(6);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingPromoAwaitingForId", kTestId);
  TEST_EQUAL(api.GetAfterBooking(lang).IsEmpty(), true, ());
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
