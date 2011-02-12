#include "../base/SRC_FIRST.hpp"

#include "mainwindow.hpp"
#include "draw_widget.hpp"
#include "update_dialog.hpp"
#include "searchwindow.hpp"
#include "classificator_tree.hpp"
#include "slider_ctrl.hpp"
#include "about.hpp"
#include "preferences_dialog.hpp"
#include "info_dialog.hpp"

#include "../defines.hpp"

#include "../map/settings.hpp"

#include "../indexer/classificator.hpp"

#include <QtGui/QDockWidget>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtCore/QFile>

#include "../base/start_mem_debug.hpp"

#define IDM_ABOUT_DIALOG        1001
#define IDM_PREFERENCES_DIALOG  1002

namespace qt
{

MainWindow::MainWindow() : m_updateDialog(0)
{
  m_pDrawWidget = new DrawWidget(this, m_storage);

  CreateNavigationBar();

  CreateClassifPanel();

  setCentralWidget(m_pDrawWidget);

  setWindowTitle(tr("MapsWithMe"));
  setWindowIcon(QIcon(":logo.png"));

#ifndef OMIM_OS_WINDOWS
  QMenu * helpMenu = new QMenu(tr("Help"), this);
  menuBar()->addMenu(helpMenu);
  helpMenu->addAction(tr("About"), this, SLOT(OnAbout()));
  helpMenu->addAction(tr("Preferences"), this, SLOT(OnPreferences()));
#else
  { // create items in the system menu
    QByteArray const aboutStr = tr("About MapsWithMe...").toLocal8Bit();
    QByteArray const prefsStr = tr("Preferences...").toLocal8Bit();
    // we use system menu for our items
    HMENU menu = ::GetSystemMenu(winId(), FALSE);
    MENUITEMINFOA item;
    item.cbSize = sizeof(MENUITEMINFOA);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    item.fType = MFT_STRING;
    item.wID = IDM_PREFERENCES_DIALOG;
    item.dwTypeData = const_cast<char *>(prefsStr.data());
    item.cch = prefsStr.size();
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
    item.wID = IDM_ABOUT_DIALOG;
    item.dwTypeData = const_cast<char *>(aboutStr.data());
    item.cch = aboutStr.size();
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
    item.fType = MFT_SEPARATOR;
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
  }
#endif

  LoadState();

  // Show intro dialog if necessary
  bool bShow = false;
  if (!Settings::Get("ShowWelcome", bShow))
  {
    QFile welcomeTextFile(GetPlatform().ReadPathForFile("welcome.html").c_str());
    if (welcomeTextFile.open(QIODevice::ReadOnly))
    {
      QByteArray text = welcomeTextFile.readAll();
      welcomeTextFile.close();

      InfoDialog welcomeDlg(tr("Welcome to MapsWithMe!"), text, this);
      QStringList buttons;
      buttons << tr("Download Maps");
      welcomeDlg.SetCustomButtons(buttons);
      welcomeDlg.exec();
    }
    Settings::Set("ShowWelcome", bool(false));

    ShowUpdateDialog();
  }
}

#if defined(Q_WS_WIN)
bool MainWindow::winEvent(MSG * msg, long * result)
{
  if (msg->message == WM_SYSCOMMAND)
  {
    if (msg->wParam == IDM_PREFERENCES_DIALOG)
    {
      OnPreferences();
      *result = 0;
      return true;
    }
    else if (msg->wParam == IDM_ABOUT_DIALOG)
    {
      OnAbout();
      *result = 0;
      return true;
    }
  }
  return false;
}
#endif

MainWindow::~MainWindow()
{
  SaveState();

  GetDownloadManager().CancelAllDownloads();
}

void MainWindow::SaveState()
{
  pair<uint32_t, uint32_t> xAndY(x(), y());
  pair<uint32_t, uint32_t> widthAndHeight(width(), height());
  Settings::Set("MainWindowXY", xAndY);
  Settings::Set("MainWindowSize", widthAndHeight);

  m_pDrawWidget->SaveState();
}

void MainWindow::LoadState()
{
  pair<uint32_t, uint32_t> xAndY;
  pair<uint32_t, uint32_t> widthAndHeight;
  bool loaded = Settings::Get("MainWindowXY", xAndY)
      && Settings::Get("MainWindowSize", widthAndHeight);
  if (loaded)
  {
    move(xAndY.first, xAndY.second);
    resize(widthAndHeight.first, widthAndHeight.second);

    loaded = m_pDrawWidget->LoadState();
  }

  if (!loaded)
  {
    showMaximized();
    m_pDrawWidget->ShowAll();
  }
}

void MainWindow::CreateClassifPanel()
{
  m_pClassifDock = new QDockWidget(tr("Classificator Bar"), this);

  ClassifTreeHolder * pCTree = new ClassifTreeHolder(m_pClassifDock, m_pDrawWidget, SLOT(Repaint()));
  pCTree->SetRoot(classif().GetMutableRoot());

  m_pClassifDock->setWidget(pCTree);

  addDockWidget(Qt::LeftDockWidgetArea, m_pClassifDock);

  // hide by default
  m_pClassifDock->hide();

  // register a hotkey to show classificator panel
  QAction * pAct = new QAction(this);
  pAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
  connect(pAct, SIGNAL(triggered()), this, SLOT(ShowClassifPanel()));
  addAction(pAct);
}

namespace 
{
  struct button_t
  {
    QString name;
    char const * icon;
    char const * slot;
  };

  void add_buttons(QToolBar * pBar, button_t buttons[], size_t count, QWidget * pReceiver)
  {
    for (size_t i = 0; i < count; ++i)
    {
      if (buttons[i].icon)
        pBar->addAction(QIcon(buttons[i].icon), buttons[i].name, pReceiver, buttons[i].slot);
      else
        pBar->addSeparator();
    }
  }
}

void MainWindow::CreateNavigationBar()
{
  QToolBar * pBar = new QToolBar(tr("Navigation Bar"), this);
  pBar->setOrientation(Qt::Vertical);
  pBar->setIconSize(QSize(32, 32));

  {
    // add view actions 1
    button_t arr[] = {
      { tr("Left"), ":/navig64/left.png", SLOT(MoveLeft()) },
      { tr("Right"), ":/navig64/right.png", SLOT(MoveRight()) },
      { tr("Up"), ":/navig64/up.png", SLOT(MoveUp()) },
      { tr("Down"), ":/navig64/down.png", SLOT(MoveDown()) },
      { tr("Show all"), ":/navig64/world.png", SLOT(ShowAll()) },
      { QString(), 0, 0 },
      { tr("Scale +"), ":/navig64/plus.png", SLOT(ScalePlus()) }
    };
    add_buttons(pBar, arr, ARRAY_SIZE(arr), m_pDrawWidget);
  }

  // add scale slider
  QClickSlider * pScale = new QClickSlider(Qt::Vertical, this);
  pScale->setRange(0, scales::GetUpperScale());
  pScale->setTickPosition(QSlider::TicksRight);

  pBar->addWidget(pScale);
  m_pDrawWidget->SetScaleControl(pScale);

  {
    // add view actions 2
    button_t arr[] = {
      { tr("Scale -"), ":/navig64/minus.png", SLOT(ScaleMinus()) },
      { QString(), 0, 0 }
    };
    add_buttons(pBar, arr, ARRAY_SIZE(arr), m_pDrawWidget);
  }

  {
    // add mainframe actions
    button_t arr[] = {
      { tr("Download Maps"), ":/navig64/download.png", SLOT(ShowUpdateDialog()) }
    };
    add_buttons(pBar, arr, ARRAY_SIZE(arr), this);
  }

  addToolBar(Qt::RightToolBarArea, pBar);
}

//void MainWindow::CreateFindTable(QLayout * pLayout)
//{
//  // find widget
//  FindEditorWnd * pEditor = new FindEditorWnd(0);
//  pLayout->addWidget(pEditor);
//
//  // find table result
//  m_pFindTable = new FindTableWnd(0, pEditor, m_pDrawWidget->GetModel());
//  pLayout->addWidget(m_pFindTable);
//  connect(m_pFindTable, SIGNAL(cellClicked(int, int)), this, SLOT(OnFeatureClicked(int, int)));
//  connect(m_pFindTable, SIGNAL(cellEntered(int, int)), this, SLOT(OnFeatureEntered(int, int)));
//}
//
//void MainWindow::OnFeatureEntered(int /*row*/, int /*col*/)
//{
//  //Feature const & p = m_pFindTable->GetFeature(row);
//
//  /// @todo highlight the feature
//}
//
//void MainWindow::OnFeatureClicked(int row, int /*col*/)
//{
//  Feature const & p = m_pFindTable->GetFeature(row);
//  m_pDrawWidget->ShowFeature(p);
//}

void MainWindow::ShowUpdateDialog()
{
  if (!m_updateDialog)
    m_updateDialog = new UpdateDialog(this, m_storage);
  m_updateDialog->ShowDialog();
  // tell download manager that we're gone...
  m_storage.Unsubscribe();
}

void MainWindow::ShowClassifPanel()
{
  m_pClassifDock->show();
}

void MainWindow::OnAbout()
{
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::OnPreferences()
{
  bool autoUpdatesEnabled = DEFAULT_AUTO_UPDATES_ENABLED;
  Settings::Get("AutomaticUpdateCheck", autoUpdatesEnabled);
  PreferencesDialog dlg(this, autoUpdatesEnabled);
  dlg.exec();
  Settings::Set("AutomaticUpdateCheck", autoUpdatesEnabled);
}

}
