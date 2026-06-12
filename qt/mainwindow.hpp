#pragma once
#include "qt/selection.hpp"

#include "map/routing_mark.hpp"

#include "storage/storage_defines.hpp"

#include "platform/location.hpp"
#include "platform/location_service/location_service.hpp"

#include "qt/build_style/build_style.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <array>
#include <memory>
#include <string>

class Framework;
class QDockWidget;
class QLabel;
class QPushButton;

namespace place_page
{
class Info;
}

namespace search
{
class Result;
}

namespace qt
{
class DrawWidget;
class PopupMenuHolder;
struct ScreenshotParams;

class MainWindow
  : public QMainWindow
  , location::LocationObserver
{
public:
  // Indices into m_Docks.
  enum DockIndex : size_t
  {
    kSearchDock = 0,
    kPlacePageDock = 1,
    kDockCount
  };

private:
  DrawWidget * m_pDrawWidget = nullptr;
  std::array<QDockWidget *, kDockCount> m_Docks;

  QPushButton * m_downloadButton = nullptr;
  QPushButton * m_retryButton = nullptr;
  QLabel * m_downloadingStatusLabel = nullptr;

  storage::CountryId m_lastCountry;

  std::unique_ptr<location::LocationService> const m_locationService;
  bool const m_screenshotMode;

  QAction * m_pMyPositionAction = nullptr;
  QAction * m_pCreateFeatureAction = nullptr;
  QAction * m_pSearchAction = nullptr;
  QAction * m_rulerAction = nullptr;

  enum LayerType : uint8_t
  {
    /// @todo Uncomment when we will integrate a traffic provider.
    // TRAFFIC = 0,
    TRANSIT = 0,  // Metro scheme
    ISOLINES,
    OUTDOORS,
    HIKING,
    CYCLING,
  };
  PopupMenuHolder * m_layers = nullptr;
  PopupMenuHolder * m_routing = nullptr;
  PopupMenuHolder * m_selection = nullptr;

  // Designer mode state; empty m_mapcssFilePath means the regular app.
  QString const m_mapcssFilePath;
  build_style::StyleInfo const m_styleInfo;
  QAction * m_pBuildStyleAction = nullptr;
  QAction * m_pRecalculateGeomIndex = nullptr;
  QAction * m_pDrawDebugRectAction = nullptr;
  QAction * m_pGetStatisticsAction = nullptr;
  QAction * m_pRunTestsAction = nullptr;
  QAction * m_pBuildPhonePackAction = nullptr;

  bool IsDesignerMode() const { return !m_mapcssFilePath.isEmpty(); }

  Q_OBJECT

public:
  MainWindow(Framework & framework, std::unique_ptr<ScreenshotParams> && screenshotParams, QRect const & screenGeometry,
             QString const & mapcssFilePath = {}, build_style::StyleInfo const & styleInfo = {});

  // Replaces the place-page dock's contents with a fresh widget for `info`
  // (Developer or User variant depending on settings::kDeveloperMode) and shows the dock.
  void ShowPlacePage(place_page::Info const & info);
  void HidePlacePage();

protected:
  Framework & GetFramework() const;

  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;
  void LocationStateModeChanged(location::EMyPositionMode mode);

  void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name, QKeySequence const & hotkey,
                       char const * slot);
  void CreateNavigationBar();
  void CreateSearchBarAndPanel();
  void CreatePlacePagePanel();
  void CreateCountryStatusControls();

  void SetLayerEnabled(LayerType type, bool enable);

#if defined(OMIM_OS_WINDOWS)
  /// to handle menu messages
  bool nativeEvent(QByteArray const & eventType, void * message, qintptr * result) override;
#endif

  void closeEvent(QCloseEvent * e) override;

protected Q_SLOTS:
#ifndef NO_DOWNLOADER
  void ShowUpdateDialog();
#endif  // NO_DOWNLOADER

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

  void OnSwitchSelectionMode(SelectionMode mode);
  void OnSwitchMwmsBordersSelectionMode();
  void OnClearSelection();

  void OnLayerEnabled(LayerType layer);

  void OnRulerEnabled();

  void OnRoutePointSelected(RouteMarkType type);
  void OnFollowRoute();
  void OnClearRoute();
  void OnRoutingSettings();

  void OnBookmarksAction();

  void OnBuildStyle();
  void OnRecalculateGeomIndex();
  void OnDebugStyle();
  void OnGetStatistics();
  void OnRunTests();
  void OnBuildPhonePackage();
};
}  // namespace qt
