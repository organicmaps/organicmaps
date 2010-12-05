#pragma once

#include "../map/storage.hpp"

#include <QtGui/QMainWindow>

#include "../base/start_mem_debug.hpp"

class QDockWidget;

namespace qt
{
  class FindTableWnd;
  class DrawWidget;

  class MainWindow : public QMainWindow
  {
    DrawWidget * m_pDrawWidget;
    QDockWidget * m_pClassifDock;
    //FindTableWnd * m_pFindTable;

    mapinfo::Storage m_Storage;

    Q_OBJECT

  public:
    MainWindow();
    virtual ~MainWindow();

  protected:
    string GetIniFile();
    void SaveState();
    void LoadState();

  protected:
    void CreateClassifPanel();
    void CreateNavigationBar();
    //void CreateFindTable(QLayout * pLayout);

  protected Q_SLOTS:
    //void OnFeatureEntered(int row, int col);
    //void OnFeatureClicked(int row, int col);
    void ShowUpdateDialog();
    void ShowClassifPanel();
  };
}

#include "../base/stop_mem_debug.hpp"
