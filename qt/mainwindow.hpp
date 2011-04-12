#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QMainWindow>

#include "../base/start_mem_debug.hpp"

class QDockWidget;

namespace qt
{
  class FindTableWnd;
  class DrawWidget;
  class UpdateDialog;

  class MainWindow : public QMainWindow
  {
    DrawWidget * m_pDrawWidget;
    QDockWidget * m_Docks[2];
    //FindTableWnd * m_pFindTable;
    UpdateDialog * m_updateDialog;

    storage::Storage m_storage;

    Q_OBJECT

  public:
    MainWindow();
    virtual ~MainWindow();

  protected:
    string GetIniFile();
    void SaveState();
    void LoadState();

  protected:
    void CreatePanelImpl(size_t i, QString const & name, QKeySequence const & hotkey,
                         char const * slot);

    void CreateClassifPanel();
    void CreateGuidePanel();

    void CreateNavigationBar();
    //void CreateFindTable(QLayout * pLayout);
  #if defined(Q_WS_WIN)
    /// to handle menu messages
    virtual bool winEvent(MSG * msg, long * result);
  #endif

  protected Q_SLOTS:
    //void OnFeatureEntered(int row, int col);
    //void OnFeatureClicked(int row, int col);
    void ShowUpdateDialog();
    void ShowClassifPanel();
    void ShowGuidePanel();
    void OnAbout();
    void OnPreferences();
  };
}

#include "../base/stop_mem_debug.hpp"
