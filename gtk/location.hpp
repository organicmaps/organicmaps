#pragma once

#include "platform/location.hpp"

#include <gio/gio.h>

#include <functional>
#include <optional>

using GClueSimple = struct _GClueSimple;

namespace gtk
{
// GeoClue may return an old cached location, and timestamps may go back. Like the desktop
// PositionFilter: fixes older than 5 minutes or older than the previous one are not current.
bool IsCurrentLocation(location::GpsInfo const & info, std::optional<location::GpsInfo> const & previous,
                       double nowSec);

// GPS through GeoClue (on FuriOS it uses the hybris GNSS source) and the magnetometer through
// iio-sensor-proxy. Updates are delivered on the main thread; nothing is called after Stop().
class LocationService
{
public:
  LocationService();
  ~LocationService();

  void Start();
  void Stop();
  bool IsStarted() const { return m_started; }

  std::function<void(location::GpsInfo const &)> m_onLocation;
  std::function<void(location::TLocationError)> m_onError;
  std::function<void(double bearingRad)> m_onCompass;

private:
  // User data of an async call: it may complete after Stop() and even after the next Start().
  struct Request;
  gpointer NewRequest();
  // Frees the request; returns nullptr if it was cancelled.
  static LocationService * TakeRequest(gpointer data);

  static void OnSimpleReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnLocationNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  void ReadLocation();
  static void OnCompassProxyReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnCompassClaimed(GObject * source, GAsyncResult * res, gpointer data);
  void StopCompass();
  static void OnCompassProperties(GDBusProxy * proxy, GVariant * changed, GStrv invalidated, gpointer data);

  bool m_started = false;
  GCancellable * m_cancellable = nullptr;
  GClueSimple * m_simple = nullptr;
  GDBusProxy * m_compass = nullptr;
  std::optional<location::GpsInfo> m_last;
};
}  // namespace gtk
