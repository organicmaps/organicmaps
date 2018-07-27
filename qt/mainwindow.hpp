#pragma once

#include "qt/draw_widget.hpp"

#include "storage/index.hpp"

#include "platform/location.hpp"
#include "platform/location_service.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <array>
#include <memory>

class Framework;
class QDockWidget;
class QLabel;
class QPushButton;
class QToolButton;

namespace search { class Result; }

namespace qt
{
class DrawWidget;

class MainWindow : public QMainWindow, location::LocationObserver
{
  DrawWidget * m_pDrawWidget = nullptr;
  // TODO(mgsergio): Make indexing more informative.
  std::array<QDockWidget *, 1> m_Docks;

  QPushButton * m_downloadButton = nullptr;
  QPushButton * m_retryButton = nullptr;
  QLabel * m_downloadingStatusLabel = nullptr;

  storage::TCountryId m_lastCountry;

  std::unique_ptr<location::LocationService> const m_locationService;

  QAction * m_pMyPositionAction = nullptr;
  QAction * m_pCreateFeatureAction = nullptr;
  QAction * m_selectionMode = nullptr;
  QAction * m_clearSelection = nullptr;
  QAction * m_pSearchAction = nullptr;
  QAction * m_trafficEnableAction = nullptr;
  QAction * m_bookmarksAction = nullptr;
  QAction * m_selectionCityBoundariesMode = nullptr;
  QToolButton * m_routePointsToolButton = nullptr;
  QAction * m_selectStartRoutePoint = nullptr;
  QAction * m_selectFinishRoutePoint = nullptr;
  QAction * m_selectIntermediateRoutePoint = nullptr;
#ifdef BUILD_DESIGNER
  QString const m_mapcssFilePath = nullptr;
  QAction * m_pBuildStyleAction = nullptr;
  QAction * m_pRecalculateGeomIndex = nullptr;
  QAction * m_pDrawDebugRectAction = nullptr;
  QAction * m_pGetStatisticsAction = nullptr;
  QAction * m_pRunTestsAction = nullptr;
  QAction * m_pBuildPhonePackAction = nullptr;
#endif // BUILD_DESIGNER

  Q_OBJECT

public:
  MainWindow(Framework & framework, bool apiOpenGLES3, QString const & mapcssFilePath = QString());

  static void SetDefaultSurfaceFormat(bool apiOpenGLES3);

protected:
  string GetIniFile();

  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;
  void LocationStateModeChanged(location::EMyPositionMode mode);

  void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                       QKeySequence const & hotkey, char const * slot);
  void CreateNavigationBar();
  void CreateSearchBarAndPanel();
  void CreateCountryStatusControls();

#if defined(Q_WS_WIN)
  /// to handle menu messages
  bool winEvent(MSG * msg, long * result) override;
#endif

  void closeEvent(QCloseEvent * e) override;

protected Q_SLOTS:
#ifndef NO_DOWNLOADER
  void ShowUpdateDialog();
#endif // NO_DOWNLOADER

  void OnPreferences();
  void OnAbout();
  void OnMyPosition();
  void OnCreateFeatureClicked();
  void OnSearchButtonClicked();
  void OnLoginMenuItem();
  void OnUploadEditsMenuItem();

  void OnBeforeEngineCreation();

  void OnDownloadClicked();
  void OnRetryDownloadClicked();

  void OnSwitchSelectionMode();
  void OnSwitchCityBoundariesSelectionMode();
  void OnClearSelection();

  void OnTrafficEnabled();
  void OnStartPointSelected();
  void OnFinishPointSelected();
  void OnIntermediatePointSelected();
  void OnFollowRoute();
  void OnClearRoute();

  void OnBookmarksAction();

#ifdef BUILD_DESIGNER
  void OnBuildStyle();
  void OnRecalculateGeomIndex();
  void OnDebugStyle();
  void OnGetStatistics();
  void OnRunTests();
  void OnBuildPhonePackage();
#endif // BUILD_DESIGNER
};
}  // namespace qt
