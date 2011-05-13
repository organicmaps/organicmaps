#pragma once

#include "../storage/storage.hpp"

#include <QtGui/QMainWindow>

class QDockWidget;

namespace qt
{
  class DrawWidget;
  class UpdateDialog;

  class MainWindow : public QMainWindow
  {
    QAction * m_pMyPositionAction;
    QAction * m_pSearchAction;
    DrawWidget * m_pDrawWidget;

    QDockWidget * m_Docks[4];

#ifdef DEBUG // code removed for desktop releases
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
    void OnSearchResult(string const & name, m2::RectD const & rect);

  protected:
#ifdef DEBUG // code removed for desktop releases
    void CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                         QKeySequence const & hotkey, char const * slot);

    void CreateClassifPanel();
    void CreateGuidePanel();
#endif // DEBUG
    void CreateNavigationBar();
    void CreateSearchBarAndPanel();

#if defined(Q_WS_WIN)
    /// to handle menu messages
    virtual bool winEvent(MSG * msg, long * result);
#endif

  protected Q_SLOTS:
    void OnSearchTextChanged(QString const &);
#ifdef DEBUG // code removed for desktop releases
    void ShowUpdateDialog();
    void ShowClassifPanel();
    void ShowGuidePanel();
#endif // DEBUG
    void OnPreferences();
    void OnAbout();
    void OnMyPosition();
    void OnSearchButtonClicked();
    void OnSearchShortcutPressed();
    void OnSearchPanelShortcutPressed();
    void OnSearchPanelItemClicked(int row, int column);
  };
}
