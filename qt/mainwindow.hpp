#pragma once

#include "storage/index.hpp"

#include "platform/location.hpp"
#include "platform/location_service.hpp"

#include "std/unique_ptr.hpp"

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QMainWindow>
#else
  #include <QtWidgets/QMainWindow>
#endif

class QDockWidget;
class QPushButton;
class QLabel;

namespace search { class Result; }

namespace qt
{
  class DrawWidget;

  class MainWindow : public QMainWindow, location::LocationObserver
  {
    QAction * m_pMyPositionAction;
    QAction * m_pCreateFeatureAction;
    QAction * m_pSearchAction;
    DrawWidget * m_pDrawWidget;

    QDockWidget * m_Docks[1];

    QPushButton * m_downloadButton;
    QPushButton * m_retryButton;
    QLabel * m_downloadingStatusLabel;
    storage::TCountryId m_lastCountry;

    unique_ptr<location::LocationService> const m_locationService;

    Q_OBJECT

  public:
    MainWindow();

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
  };
}
