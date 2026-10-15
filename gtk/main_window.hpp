#pragma once

#include "app.hpp"
#include "location.hpp"
#include "map_view.hpp"

#include "platform/location.hpp"

#include <memory>
#include <string>

namespace gtk
{
class MainWindow
{
public:
  explicit MainWindow(AdwApplication * app);
  ~MainWindow();

  GtkWindow * Window() const { return GTK_WINDOW(m_window); }

  void Present();
  // Stops everything that calls back into this object and destroys the window; the object itself
  // may be deleted after the main loop has dispatched what is left.
  void Shutdown();
  void Toast(std::string const & text);

  void ShowMenu();
  void StartLocation();
  void ToggleMapControls();

private:
  void BuildUi();
  void BuildMapControls(GtkWidget * overlay);
  GtkWidget * BuildBottomMenu();
  void QueueMapInsetsUpdate();
  void UpdateMapInsets();
  void LoadCss();
  void SetupFramework();
  void OnMyPositionModeChanged(location::EMyPositionMode mode);
  void SyncNightMode();
  void OnSuspendedChanged();

  AdwApplicationWindow * m_window = nullptr;
  AdwToastOverlay * m_toasts = nullptr;

  std::unique_ptr<MapView> m_map;
  LocationService m_location;

  // Map controls.
  GtkWidget * m_rightButtons = nullptr;
  GtkWidget * m_myPositionButton = nullptr;
  GtkWidget * m_bottomMenu = nullptr;
  GtkWidget * m_mapAttribution = nullptr;

  gulong m_darkHandler = 0;
  guint m_insetsIdle = 0;

  bool m_suspended = false;
  bool m_controlsHidden = false;
  int m_lastBottomInset = -1;
  bool m_shutdown = false;
};
}  // namespace gtk
