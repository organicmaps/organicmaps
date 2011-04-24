#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QMainWindow>

class QDockWidget;

namespace qt
{
  class FindTableWnd;
  class DrawWidget;
  class UpdateDialog;

  class MainWindow : public QMainWindow
  {
    QAction * m_pMyPosition;
    DrawWidget * m_pDrawWidget;

#ifdef DEBUG // code removed for desktop releases
    QDockWidget * m_Docks[2];
    //FindTableWnd * m_pFindTable;
    UpdateDialog * m_updateDialog;
#endif // DEBUG

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
#ifdef DEBUG // code removed for desktop releases
    void CreatePanelImpl(size_t i, QString const & name, QKeySequence const & hotkey,
                         char const * slot);

    void CreateClassifPanel();
    void CreateGuidePanel();
#endif // DEBUG
    void CreateNavigationBar();
    //void CreateFindTable(QLayout * pLayout);

#if defined(Q_WS_WIN)
    /// to handle menu messages
    virtual bool winEvent(MSG * msg, long * result);
#endif

  protected Q_SLOTS:
    //void OnFeatureEntered(int row, int col);
    //void OnFeatureClicked(int row, int col);
#ifdef DEBUG // code removed for desktop releases
    void ShowUpdateDialog();
    void ShowClassifPanel();
    void ShowGuidePanel();
    void OnPreferences();
#endif // DEBUG
    void OnAbout();
    void OnMyPosition();
  };
}
