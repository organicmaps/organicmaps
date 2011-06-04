#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QMainWindow>

class QDockWidget;

namespace search { class Result; }

namespace qt
{
  class DrawWidget;
  class UpdateDialog;

  class MainWindow : public QMainWindow
  {
    QAction * m_pMyPositionAction;
    QAction * m_pSearchAction;
    DrawWidget * m_pDrawWidget;

    QDockWidget * m_Docks[3];

#ifndef NO_DOWNLOADER
    UpdateDialog * m_updateDialog;
#endif // NO_DOWNLOADER

    storage::Storage m_storage;

    Q_OBJECT

  public:
    MainWindow();
    virtual ~MainWindow();

  protected:
    string GetIniFile();
    void SaveState();
    void LoadState();

  private:
    void OnLocationFound();

  protected:
#ifndef NO_DOWNLOADER
    void CreateClassifPanel();
    void CreateGuidePanel();
#endif // NO_DOWNLOADER
    void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                         QKeySequence const & hotkey, char const * slot);
    void CreateNavigationBar();
    void CreateSearchBarAndPanel();

#if defined(Q_WS_WIN)
    /// to handle menu messages
    virtual bool winEvent(MSG * msg, long * result);
#endif

  protected Q_SLOTS:
#ifndef NO_DOWNLOADER
    void ShowUpdateDialog();
    void ShowClassifPanel();
    void ShowGuidePanel();
#endif // NO_DOWNLOADER
    void OnPreferences();
    void OnAbout();
    void OnMyPosition();
    void OnSearchButtonClicked();
  };
}
