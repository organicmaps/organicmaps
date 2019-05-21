#include "testing/testing.hpp"

#include "partners_api/cross_reference_api.hpp"

#include "platform/settings.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include <algorithm>
#include <memory>

using namespace eye;
using namespace platform::tests_support;

namespace
{
std::string const kTestOsmId = "TestOsmId";

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

class DelegateForTesting : public cross_reference::Api::Delegate
{
public:
  std::string GetCityOsmId(m2::PointD const &) override { return kTestOsmId; }
};
}  // namespace

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, CrossReference_NeedToShow)
{
  cross_reference::Api api;
  Info info;
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
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();
  TEST_EQUAL(api.NeedToShow(), false, ());

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

  info.m_crossReferences.m_transitionToBookingTime = Clock::now() - std::chrono::hours(2);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();
  TEST_EQUAL(api.NeedToShow(), false, ());

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

  info.m_crossReferences.m_transitionToBookingTime = Clock::now() - std::chrono::minutes(6);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();
  TEST_EQUAL(api.NeedToShow(), true, ());
}

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, CrossReference_GetCrossReferenceLinkAfterBooking)
{
  cross_reference::Api api;
  api.SetDelegate(std::make_unique<DelegateForTesting>());
  Info info;
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
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();

  std::string result{};
  api.GetCrossReferenceLinkAfterBooking([&result](std::string const & url)
  {
    result = url;
    testing::Notify();
  });

  testing::Wait();
  TEST(result.empty(), ());

  auto eventTime = Clock::now() - std::chrono::hours(2);
  {
    MapObject poi;
    poi.SetBestType("tourism-hotel");
    poi.SetPos({53.652005, 108.143448});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 30 * 3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = eventTime;
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::BookingBook;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  info.m_crossReferences.m_transitionToBookingTime = Clock::now() - std::chrono::hours(2);
  EyeForTesting::SetInfo(info);
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();

  result = {};
  api.GetCrossReferenceLinkAfterBooking([&result](std::string const & url)
  {
    result = url;
    testing::Notify();
  });

  testing::Wait();
  TEST(result.empty(), ());

  eventTime = Clock::now() - std::chrono::minutes(6);
  {
    MapObject poi;
    poi.SetBestType("tourism-hotel");
    poi.SetPos({53.653005, 108.143548});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 20 * 3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = eventTime;
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

  info.m_crossReferences.m_transitionToBookingTime = eventTime;
  EyeForTesting::SetInfo(info);
  settings::Set("BookingCrossReferenceIsAwaiting", true);
  api.OnEnterForeground();

  result = {};
  api.GetCrossReferenceLinkAfterBooking([&result](std::string const & url)
  {
    result = url;
    testing::Notify();
  });

  testing::Wait();
  TEST_NOT_EQUAL(result.find(kTestOsmId, 0), std::string::npos, ());
}

UNIT_CLASS_TEST(ScopedEyeWithAsyncGuiThread, CrossReference_GetCrossReferenceCityGallery)
{
  {}
  cross_reference::Api api("http://localhost:34568/gallery/city/");
  api.SetDelegate(std::make_unique<DelegateForTesting>());

  {
    cross_reference::CityGallery result{};
    api.GetCrossReferenceCityGallery(kTestOsmId, [&result](cross_reference::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.size(), 2, ());
  }
  {
    cross_reference::CityGallery result{};
    m2::PointD pt;
    api.GetCrossReferenceCityGallery(pt, [&result](cross_reference::CityGallery const & gallery)
    {
      result = gallery;
      testing::Notify();
    });

    testing::Wait();
    TEST_EQUAL(result.size(), 2, ());
  }
}
