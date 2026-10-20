#include "testing/testing.hpp"

#include "platform/location_service/qt_location_service.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QMetaObject>

#include <functional>

namespace qt_location_service_tests
{
struct ProviderState
{
  int starts = 0;
  int stops = 0;
  int requests = 0;
  bool running = false;
};

class Source final : public QGeoPositionInfoSource
{
public:
  explicit Source(ProviderState & state) : QGeoPositionInfoSource(nullptr), m_state(state) {}
  int minimumUpdateInterval() const override { return 1000; }
  PositioningMethods supportedPositioningMethods() const override { return m_methods; }
  Error error() const override { return NoError; }
  QGeoPositionInfo lastKnownPosition(bool) const override { return {}; }
  void requestUpdate(int) override { ++m_state.requests; }
  void startUpdates() override
  {
    ++m_state.starts;
    m_state.running = true;
  }
  void stopUpdates() override
  {
    ++m_state.stops;
    m_state.running = false;
  }

  void Fix(QDateTime const & timestamp = QDateTime::currentDateTimeUtc())
  {
    emit positionUpdated(QGeoPositionInfo(QGeoCoordinate(47, 8), timestamp));
  }
  void EmitError(Error error) { emit errorOccurred(error); }
  void Disable()
  {
    m_methods = NoPositioningMethods;
    emit supportedPositioningMethodsChanged();
  }

private:
  ProviderState & m_state;
  PositioningMethods m_methods = AllPositioningMethods;
};

struct Observer final : location::LocationObserver
{
  int fixes = 0;
  int errors = 0;
  location::TLocationError lastError = location::ENoError;
  std::function<void()> onFix;
  std::function<void()> onError;
  void OnLocationUpdated(location::GpsInfo const &) override
  {
    ++fixes;
    if (onFix)
      onFix();
  }
  void OnLocationError(location::TLocationError error) override
  {
    ++errors;
    lastError = error;
    if (onError)
      onError();
  }
};

UNIT_TEST(QtLocationService_StopBeforeFirstFix)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  service.Start();
  TEST(state.running, ());
  TEST_EQUAL(state.requests, 0, ("All positioning work must be cancellable by stopUpdates"));
  service.Stop();
  TEST(!state.running, ());
  TEST_EQUAL(state.stops, 1, ());
  TEST(!service.findChild<QTimer *>()->isActive(), ());
  service.Stop();
  TEST_EQUAL(state.stops, 1, ());
}

UNIT_TEST(QtLocationService_StartIsIdempotent)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  service.Start();
  auto * timer = service.findChild<QTimer *>();
  TEST(timer, ());
  auto const timerId = timer->timerId();
  service.Start();
  TEST_EQUAL(state.starts, 1, ());
  TEST_EQUAL(state.stops, 0, ());
  TEST_EQUAL(timer->timerId(), timerId, ("A repeated start must retain the acquisition deadline"));

  source->Fix();
  service.Start();
  TEST_EQUAL(state.starts, 1, ());
  TEST_EQUAL(state.stops, 0, ());
  TEST(!timer->isActive(), ("A repeated start must not rearm the deadline after acquisition"));
}

UNIT_TEST(QtLocationService_RejectedFixKeepsDeadline)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  service.Start();
  auto * timer = service.findChild<QTimer *>();
  TEST(timer, ());
  auto const now = QDateTime::currentDateTimeUtc();
  QMetaObject::invokeMethod(source, [source, now]
  {
    source->Fix(QDateTime{});
    source->Fix(now.addSecs(-3600));
  }, Qt::QueuedConnection);
  QCoreApplication::processEvents();
  TEST_EQUAL(observer.fixes, 0, ());
  TEST(timer->isActive(), ("Invalid and expired cached fixes must not end acquisition"));

  source->Fix(now.addSecs(-60));
  TEST_EQUAL(observer.fixes, 1, ());
  TEST(!timer->isActive(), ());
  service.Stop();
  service.Start();
  source->Fix(now.addSecs(-120));
  TEST_EQUAL(observer.fixes, 1, ());
  TEST(timer->isActive(), ("An out-of-order fix must not end acquisition after restart"));
  source->Fix(now);
  TEST_EQUAL(observer.fixes, 2, ());
  TEST(!timer->isActive(), ());
}

UNIT_TEST(QtLocationService_StopAfterErrorOrProviderDisabled)
{
  for (bool const disabled : {false, true})
  {
    ProviderState state;
    Observer observer;
    auto * source = new Source(state);
    QtLocationService service(observer, source);
    service.Start();
    source->Fix();
    source->EmitError(QGeoPositionInfoSource::NoError);
    TEST_EQUAL(observer.errors, 0, ());
    if (disabled)
      source->Disable();
    else
      source->EmitError(QGeoPositionInfoSource::AccessError);
    TEST_EQUAL(observer.errors, 1, ());
    service.Stop();
    TEST(!state.running, ());
    TEST_EQUAL(state.stops, 1, ());
  }
}

UNIT_TEST(QtLocationService_IgnoresStoppedCallbacks)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  service.Start();
  QMetaObject::invokeMethod(source, [source]
  {
    source->Fix();
    source->EmitError(QGeoPositionInfoSource::AccessError);
    source->Disable();
  }, Qt::QueuedConnection);
  service.Stop();
  QCoreApplication::processEvents();
  TEST_EQUAL(observer.fixes, 0, ());
  TEST_EQUAL(observer.errors, 0, ());
  TEST_EQUAL(state.starts, 1, ());
}

UNIT_TEST(QtLocationService_ObserverCanStopAndRestart)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  observer.onFix = [&] { service.Stop(); };
  service.Start();
  source->Fix();
  TEST(!state.running, ());
  TEST_EQUAL(state.starts, 1, ());
  observer.onFix = {};
  service.Start();
  source->Fix();
  TEST(state.running, ());
  TEST_EQUAL(observer.fixes, 2, ());
  TEST(!service.findChild<QTimer *>()->isActive(), ());
  service.Start();
  TEST_EQUAL(state.starts, 2, ());
  TEST_EQUAL(state.stops, 1, ());
}

UNIT_TEST(QtLocationService_FirstFixDeadlineStopsProvider)
{
  ProviderState state;
  Observer observer;
  auto * source = new Source(state);
  QtLocationService service(observer, source);
  auto * timer = service.findChild<QTimer *>();
  TEST(timer, ());
  TEST_EQUAL(timer->interval(), 30 * 60 * 1000, ());
  timer->setInterval(0);
  QEventLoop loop;
  observer.onError = [&] { loop.quit(); };
  service.Start();
  QTimer::singleShot(1000, &loop, &QEventLoop::quit);
  loop.exec();
  TEST_EQUAL(observer.errors, 1, ());
  TEST_EQUAL(observer.lastError, location::ETimeout, ());
  TEST(!state.running, ());
  TEST_EQUAL(state.stops, 1, ());
  source->Fix();
  TEST_EQUAL(observer.fixes, 0, ());
}

UNIT_TEST(QtLocationService_DestructionStopsProvider)
{
  ProviderState state;
  Observer observer;
  {
    QtLocationService service(observer, new Source(state));
    service.Start();
  }
  TEST(!state.running, ());
  TEST_EQUAL(state.stops, 1, ());
}
}  // namespace qt_location_service_tests
