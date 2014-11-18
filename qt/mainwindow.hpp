#pragma once

#include "../map/location_state.hpp"
#include "../platform/location_service.hpp"

#include "../std/unique_ptr.hpp"

#include <Qt>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QMainWindow>
#else
  #include <QtWidgets/QMainWindow>
#endif



class QDockWidget;

namespace search { class Result; }

namespace qt
{
#ifndef USE_DRAPE
  class DrawWidget;
#else
  class DrapeSurface;
#endif // USE_DRAPE

  class MainWindow : public QMainWindow, location::LocationObserver
  {
    QAction * m_pMyPositionAction;
    QAction * m_pSearchAction;
#ifndef USE_DRAPE
    DrawWidget * m_pDrawWidget;
#else
    DrapeSurface * m_pDrawWidget;
#endif // USE_DRAPE

    QDockWidget * m_Docks[1];

    unique_ptr<location::LocationService> const m_locationService;

    Q_OBJECT

  public:
    MainWindow();
    virtual ~MainWindow();

    virtual void OnLocationError(location::TLocationError errorCode);
    virtual void OnLocationUpdated(location::GpsInfo const & info);

  protected:
    string GetIniFile();
    void SaveState();
    void LoadState();

    void LocationStateModeChanged(location::State::Mode mode);

  protected:
    void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                         QKeySequence const & hotkey, char const * slot);
    void CreateNavigationBar();
    void CreateSearchBarAndPanel();

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
    void OnSearchButtonClicked();
  };
}
