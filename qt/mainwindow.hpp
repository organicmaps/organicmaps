#pragma once

#include "storage/index.hpp"

#include "platform/location.hpp"
#include "platform/location_service.hpp"

#include "std/array.hpp"
#include "std/unique_ptr.hpp"

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QMainWindow>
#else
  #include <QtWidgets/QMainWindow>
#endif

class Framework;
class QDockWidget;
class QLabel;
class QPushButton;
class TrafficMode;

namespace search { class Result; }

namespace qt
{
  class DrawWidget;

  class MainWindow : public QMainWindow, location::LocationObserver
  {
    QAction * m_pMyPositionAction;
    QAction * m_pCreateFeatureAction;
    QAction * m_selectionMode;
    QAction * m_clearSelection;
    QAction * m_pSearchAction;
    QAction * m_trafficEnableAction;
    QAction * m_saveTrafficSampleAction;
    QAction * m_quitTrafficModeAction;
    DrawWidget * m_pDrawWidget;

    // TODO(mgsergio): Make indexing more informative.
    array<QDockWidget *, 2> m_Docks;

    QPushButton * m_downloadButton;
    QPushButton * m_retryButton;
    QLabel * m_downloadingStatusLabel;
    storage::TCountryId m_lastCountry;

    unique_ptr<location::LocationService> const m_locationService;

    // This object is managed by Qt memory system.
    TrafficMode * m_trafficMode = nullptr;

    Q_OBJECT

  public:
    MainWindow(Framework & framework);

    virtual void OnLocationError(location::TLocationError errorCode);
    virtual void OnLocationUpdated(location::GpsInfo const & info);

  protected:
    string GetIniFile();

    void LocationStateModeChanged(location::EMyPositionMode mode);

  protected:
    void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                         QKeySequence const & hotkey, char const * slot);
    void CreateNavigationBar();
    void CreateSearchBarAndPanel();
    void CreateCountryStatusControls();

    void CreateTrafficPanel(string const & dataFilePath, string const & sampleFilePath);
    void DestroyTrafficPanel();

#if defined(Q_WS_WIN)
    /// to handle menu messages
    virtual bool winEvent(MSG * msg, long * result);
#endif

    virtual void closeEvent(QCloseEvent * e);

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
    void OnClearSelection();

    void OnTrafficEnabled();
    void OnOpenTrafficSample();
    void OnSaveTrafficSample();
    void OnQuitTrafficMode();
  };
}
