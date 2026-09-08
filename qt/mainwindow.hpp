#pragma once
#include "qt/selection.hpp"

#include "map/routing_mark.hpp"

#include "storage/storage_defines.hpp"

#include "platform/location.hpp"
#include "platform/location_service/location_service.hpp"

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

signals:
  void positionModeChanged(location::EMyPositionMode mode);
  void infoChanged();

private:
  QString m_title, m_subTitle, m_address, m_wikipedia, m_wikimedia, m_description, m_openingHours, m_cuisines, m_phone,
      m_operator, m_website, m_email, m_facebook, m_instagram, m_twitter, m_vk, m_line, m_level;
  bool m_bookmark = false, m_atm = false, m_wifi = false;

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

#ifdef BUILD_DESIGNER
  QString const m_mapcssFilePath = nullptr;
  QAction * m_pBuildStyleAction = nullptr;
  QAction * m_pRecalculateGeomIndex = nullptr;
  QAction * m_pDrawDebugRectAction = nullptr;
  QAction * m_pGetStatisticsAction = nullptr;
  QAction * m_pRunTestsAction = nullptr;
  QAction * m_pBuildPhonePackAction = nullptr;
#endif  // BUILD_DESIGNER

  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")

  Q_PROPERTY(QString title MEMBER m_title NOTIFY infoChanged)
  Q_PROPERTY(QString subTitle MEMBER m_subTitle NOTIFY infoChanged)
  Q_PROPERTY(QString address MEMBER m_address NOTIFY infoChanged)
  Q_PROPERTY(bool bookmark MEMBER m_bookmark NOTIFY infoChanged)
  Q_PROPERTY(QString wikipedia MEMBER m_wikipedia NOTIFY infoChanged)
  Q_PROPERTY(QString wikimedia MEMBER m_wikimedia NOTIFY infoChanged)
  Q_PROPERTY(QString description MEMBER m_description NOTIFY infoChanged)
  Q_PROPERTY(QString openingHours MEMBER m_openingHours NOTIFY infoChanged)
  Q_PROPERTY(QString cuisines MEMBER m_cuisines NOTIFY infoChanged)
  Q_PROPERTY(QString phone MEMBER m_phone NOTIFY infoChanged)
  Q_PROPERTY(QString operator MEMBER m_operator NOTIFY infoChanged)
  Q_PROPERTY(bool atm MEMBER m_atm NOTIFY infoChanged)
  Q_PROPERTY(bool wifi MEMBER m_wifi NOTIFY infoChanged)
  Q_PROPERTY(QString website MEMBER m_website NOTIFY infoChanged)
  Q_PROPERTY(QString email MEMBER m_email NOTIFY infoChanged)
  Q_PROPERTY(QString facebook MEMBER m_facebook NOTIFY infoChanged)
  Q_PROPERTY(QString instagram MEMBER m_instagram NOTIFY infoChanged)
  Q_PROPERTY(QString twitter MEMBER m_twitter NOTIFY infoChanged)
  Q_PROPERTY(QString vk MEMBER m_vk NOTIFY infoChanged)
  Q_PROPERTY(QString line MEMBER m_line NOTIFY infoChanged)
  Q_PROPERTY(QString level MEMBER m_level NOTIFY infoChanged)

public:
  MainWindow(Framework & framework, std::unique_ptr<ScreenshotParams> && screenshotParams, QRect const & screenGeometry
#ifdef BUILD_DESIGNER
             ,
             QString const & mapcssFilePath = QString()
#endif
  );

  // Replaces the place-page dock's contents with a fresh widget for `info`
  // (Developer or User variant depending on settings::kDeveloperMode) and shows the dock.
  void ShowPlacePage(place_page::Info const & info);
  void HidePlacePage();

  Q_INVOKABLE QAction * getMyPositionAction() const { return m_pMyPositionAction; }

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

#ifdef BUILD_DESIGNER
  void OnBuildStyle();
  void OnRecalculateGeomIndex();
  void OnDebugStyle();
  void OnGetStatistics();
  void OnRunTests();
  void OnBuildPhonePackage();
#endif  // BUILD_DESIGNER
};
}  // namespace qt
