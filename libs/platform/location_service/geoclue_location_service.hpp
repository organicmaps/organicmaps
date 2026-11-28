#pragma once

#include "platform/location_service/location_service.hpp"
#include "platform/location_service/position_filter.hpp"

#include <gio/gio.h>

#include <chrono>
#include <string>

using GClueSimple = struct _GClueSimple;
using GClueClient = struct _GClueClient;
using GClueLocation = struct _GClueLocation;

namespace location
{
// GeoClue through xdg-desktop-portal in Flatpak when built with libgeoclue 2.7.2 or newer, and the system
// service otherwise. Callbacks come from the GLib main context: a GTK app runs it, and so does Qt on Linux
// (QEventDispatcherGlib by default).
// Start(), Stop() and destruction must happen on the thread of that context. Nothing is reported after Stop().
class GeoClueLocationService final : public LocationService
{
public:
  // Allow receivers without assisted GPS up to 30 minutes to acquire a usable fix.
  static constexpr std::chrono::milliseconds kFirstFixTimeout = std::chrono::minutes(30);

  // desktopId is the name of the app's .desktop file, the system service authorizes the app by it.
  GeoClueLocationService(LocationObserver & observer, std::string desktopId,
                         std::chrono::milliseconds firstFixTimeout = kFirstFixTimeout);
  ~GeoClueLocationService() override;
  // Async callbacks keep the address of the service.
  GeoClueLocationService(GeoClueLocationService const &) = delete;
  GeoClueLocationService & operator=(GeoClueLocationService const &) = delete;

  void Start() override;
  void Stop() override;
  bool IsStarted() const { return m_started; }

private:
  // User data of an async call: it may complete after Stop() and even after the next Start().
  struct Request;
  gpointer NewRequest(GCancellable * cancellable = nullptr);
  // Frees the request; returns nullptr if it was cancelled.
  static GeoClueLocationService * TakeRequest(gpointer data);

  void Fail(TLocationError error);
  void LocationError(GError * error);
  static gboolean OnFirstFixTimeout(gpointer data);
  static void OnClientReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnClientStarted(GObject * source, GAsyncResult * res, gpointer data);
  static void OnClientPropertiesReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnLocationUpdated(GClueClient * client, char const * oldPath, char const * newPath, gpointer data);
  void LoadLocation(char const * path);
  static void OnLocationReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnSimpleReady(GObject * source, GAsyncResult * res, gpointer data);
  static void OnClientActiveNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  bool CheckClientActive(bool active);
  static void OnLocationNotify(GObject * obj, GParamSpec * pspec, gpointer data);
  void ReadLocation(GClueLocation * loc);

  std::string const m_desktopId;
  std::chrono::milliseconds const m_firstFixTimeout;
  bool m_started = false;
  guint m_firstFixTimer = 0;
  GCancellable * m_cancellable = nullptr;
  GClueSimple * m_simple = nullptr;
  GClueClient * m_client = nullptr;
  bool m_clientStarted = false;
  GCancellable * m_locationRequest = nullptr;
  // Kept across Stop() and Start(): a restart must not accept a fix older than the one already shown.
  PositionFilter m_filter;
};
}  // namespace location
