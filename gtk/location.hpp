#pragma once

#include "platform/location.hpp"

#include <gio/gio.h>

#include <functional>
#include <optional>

using GClueSimple = struct _GClueSimple;
using GClueClient = struct _GClueClient;
using GClueLocation = struct _GClueLocation;

namespace gtk
{
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
  gpointer NewRequest(GCancellable * cancellable = nullptr);
  // Frees the request; returns nullptr if it was cancelled.
  static LocationService * TakeRequest(gpointer data);

  void LocationError(GError * error);
  static void OnClientReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnClientStarted(GObject * source, GAsyncResult * res, gpointer data);
  static void OnLocationUpdated(GClueClient * client, char const * oldPath, char const * newPath, gpointer data);
  void LoadLocation(char const * path);
  static void OnLocationReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnSimpleReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnClientActiveNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  bool CheckClientActive();
  static void OnLocationNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  void ReadLocation(GClueLocation * loc);
  static void OnCompassProxyReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnCompassClaimed(GObject * source, GAsyncResult * res, gpointer data);
  static void OnCompassOwnerNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  void ClaimCompass();
  void StopCompass();
  void ReadCompassHeading();
  void OnCompassHeading(double heading);
  static void OnCompassProperties(GDBusProxy * proxy, GVariant * changed, GStrv invalidated, gpointer data);

  bool m_started = false;
  GCancellable * m_cancellable = nullptr;
  GClueSimple * m_simple = nullptr;
  GClueClient * m_client = nullptr;
  bool m_clientStarted = false;
  GCancellable * m_locationRequest = nullptr;
  GDBusProxy * m_compass = nullptr;
  GCancellable * m_compassRequest = nullptr;
  bool m_compassClaimed = false;
  std::optional<location::GpsInfo> m_last;
};
}  // namespace gtk
