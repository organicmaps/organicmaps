#include "mainwindow.hpp"
#include "draw_widget.hpp"
#include "slider_ctrl.hpp"
#include "about.hpp"

#ifdef DEBUG
#include "info_dialog.hpp"
#include "update_dialog.hpp"
#include "preferences_dialog.hpp"
#include "classificator_tree.hpp"
#include "guide_page.hpp"

#include "../indexer/classificator.hpp"
#endif

#include "../defines.hpp"

#include "../map/settings.hpp"

#include <QtGui/QDockWidget>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>

#ifdef DEBUG
#include <QtCore/QFile>
#endif

#define IDM_ABOUT_DIALOG        1001

#ifdef DEBUG // code removed for desktop releases
#include "update_dialog.hpp"
#include "searchwindow.hpp"
#include "classificator_tree.hpp"
#include "preferences_dialog.hpp"
#include "info_dialog.hpp"
#include "guide_page.hpp"

#include "../indexer/classificator.hpp"

#include <QtCore/QFile>

#define IDM_PREFERENCES_DIALOG  1002

#endif // DEBUG

namespace qt
{

MainWindow::MainWindow()
#ifdef DEBUG // code removed for desktop releases
  : m_updateDialog(0)
#endif // DEBUG
{
  m_pDrawWidget = new DrawWidget(this, m_storage);

  CreateNavigationBar();

#ifdef DEBUG // code removed for desktop releases
  CreateClassifPanel();
  CreateGuidePanel();
#endif // DEBUG
  setCentralWidget(m_pDrawWidget);

  setWindowTitle(tr("MapsWithMe"));
  setWindowIcon(QIcon(":logo.png"));

#ifndef OMIM_OS_WINDOWS
  QMenu * helpMenu = new QMenu(tr("Help"), this);
  menuBar()->addMenu(helpMenu);
  helpMenu->addAction(tr("About"), this, SLOT(OnAbout()));
#ifdef DEBUG // code removed for desktop releases
  helpMenu->addAction(tr("Preferences"), this, SLOT(OnPreferences()));
#endif // DEBUG
#else
  {
    // create items in the system menu
    HMENU menu = ::GetSystemMenu(winId(), FALSE);
    MENUITEMINFOA item;
    item.cbSize = sizeof(MENUITEMINFOA);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    item.fType = MFT_STRING;
 #ifdef DEBUG // code removed for desktop releases
    item.wID = IDM_PREFERENCES_DIALOG;
    QByteArray const prefsStr = tr("Preferences...").toLocal8Bit();
    item.dwTypeData = const_cast<char *>(prefsStr.data());
    item.cch = prefsStr.size();
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
 #endif // DEBUG
    item.wID = IDM_ABOUT_DIALOG;
    QByteArray const aboutStr = tr("About MapsWithMe...").toLocal8Bit();
    item.dwTypeData = const_cast<char *>(aboutStr.data());
    item.cch = aboutStr.size();
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
    item.fType = MFT_SEPARATOR;
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
  }
#endif

  LoadState();

#ifdef DEBUG // code removed for desktop releases
  // Show intro dialog if necessary
  bool bShow = true;
  (void)Settings::Get("ShowWelcome", bShow);

  if (bShow)
  {
    QFile welcomeTextFile(GetPlatform().ReadPathForFile("welcome.html").c_str());

    bool bShowUpdateDialog = true;

    if (welcomeTextFile.open(QIODevice::ReadOnly))
    {
      QByteArray text = welcomeTextFile.readAll();
      welcomeTextFile.close();

      InfoDialog welcomeDlg(tr("Welcome to MapsWithMe!"), text, this, QStringList(tr("Download Maps")));
      if (welcomeDlg.exec() == QDialog::Rejected)
        bShowUpdateDialog = false;
    }
    Settings::Set("ShowWelcome", false);

    if (bShowUpdateDialog)
      ShowUpdateDialog();
  }
#endif // DEBUG
}

#if defined(Q_WS_WIN)
bool MainWindow::winEvent(MSG * msg, long * result)
{
  if (msg->message == WM_SYSCOMMAND)
  {
    switch (msg->wParam)
    {
#ifdef DEBUG // code removed for desktop releases
    case IDM_PREFERENCES_DIALOG:
      OnPreferences();
      *result = 0;
      return true;
#endif
    case IDM_ABOUT_DIALOG:
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
#ifdef DEBUG // code removed for desktop releases
  GetDownloadManager().CancelAllDownloads();
#endif
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
  bool loaded = Settings::Get("MainWindowXY", xAndY) &&
                Settings::Get("MainWindowSize", widthAndHeight);
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
  else
    m_pDrawWidget->UpdateNow();
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

  struct hotkey_t
  {
    int key;
    char const * slot;
  };
}

void MainWindow::CreateNavigationBar()
{
  QToolBar * pBar = new QToolBar(tr("Navigation Bar"), this);
  pBar->setOrientation(Qt::Vertical);
  pBar->setIconSize(QSize(32, 32));

  {
    // add navigation hot keys
    hotkey_t arr[] = {
      { Qt::Key_Left, SLOT(MoveLeft()) },
      { Qt::Key_Right, SLOT(MoveRight()) },
      { Qt::Key_Up, SLOT(MoveUp()) },
      { Qt::Key_Down, SLOT(MoveDown()) },
      { Qt::Key_Equal, SLOT(ScalePlus()) },
      { Qt::Key_Minus, SLOT(ScaleMinus()) },
      { Qt::ALT + Qt::Key_Equal, SLOT(ScalePlusLight()) },
      { Qt::ALT + Qt::Key_Minus, SLOT(ScaleMinusLight()) },
      { Qt::Key_A, SLOT(ShowAll()) }
    };

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    {
      QAction * pAct = new QAction(this);
      pAct->setShortcut(QKeySequence(arr[i].key));
      connect(pAct, SIGNAL(triggered()), m_pDrawWidget, arr[i].slot);
      addAction(pAct);
    }
  }

  {
    // add my position button with "checked" behavior
    m_pMyPosition = pBar->addAction(QIcon(":/navig64/location.png"),
                                           tr("My Position"),
                                           this,
                                           SLOT(OnMyPosition()));
    m_pMyPosition->setCheckable(true);
    m_pMyPosition->setToolTip(tr("My Position"));

    // add view actions 1
    button_t arr[] = {
//      { tr("Left"), ":/navig64/left.png", SLOT(MoveLeft()) },
//      { tr("Right"), ":/navig64/right.png", SLOT(MoveRight()) },
//      { tr("Up"), ":/navig64/up.png", SLOT(MoveUp()) },
//      { tr("Down"), ":/navig64/down.png", SLOT(MoveDown()) },
      { QString(), 0, 0 },
      { tr("Show all"), ":/navig64/world.png", SLOT(ShowAll()) },
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
      { tr("Scale -"), ":/navig64/minus.png", SLOT(ScaleMinus()) }
    };
    add_buttons(pBar, arr, ARRAY_SIZE(arr), m_pDrawWidget);
  }
#ifdef DEBUG // code removed for desktop releases
  {
    // add mainframe actions
    button_t arr[] = {
      { QString(), 0, 0 },
      { tr("Download Maps"), ":/navig64/download.png", SLOT(ShowUpdateDialog()) }
    };
    add_buttons(pBar, arr, ARRAY_SIZE(arr), this);
  }
#endif // DEBUG
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
void MainWindow::OnAbout()
{
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::OnLocationFound()
{
  m_pMyPosition->setIcon(QIcon(":/navig64/location.png"));
  m_pMyPosition->setToolTip(tr("My Position"));
}

void MainWindow::OnMyPosition()
{
  if (m_pMyPosition->isChecked())
  {
    m_pMyPosition->setIcon(QIcon(":/navig64/location-search.png"));
    m_pMyPosition->setToolTip(tr("Looking for position..."));
    m_pDrawWidget->OnEnableMyPosition(boost::bind(&MainWindow::OnLocationFound, this));
  }
  else
  {
    m_pMyPosition->setIcon(QIcon(":/navig64/location.png"));
    m_pMyPosition->setToolTip(tr("My Position"));
    m_pDrawWidget->OnDisableMyPosition();
  }
}

#ifdef DEBUG // code removed for desktop releases
void MainWindow::ShowUpdateDialog()
{
  if (!m_updateDialog)
    m_updateDialog = new UpdateDialog(this, m_storage);
  m_updateDialog->ShowDialog();
}

void MainWindow::ShowClassifPanel()
{
  m_Docks[0]->show();
}

void MainWindow::ShowGuidePanel()
{
  m_Docks[1]->show();
}

void MainWindow::OnPreferences()
{
  bool autoUpdatesEnabled = DEFAULT_AUTO_UPDATES_ENABLED;
  Settings::Get("AutomaticUpdateCheck", autoUpdatesEnabled);

  PreferencesDialog dlg(this, autoUpdatesEnabled);
  dlg.exec();

  Settings::Set("AutomaticUpdateCheck", autoUpdatesEnabled);
}

void MainWindow::CreateClassifPanel()
{
  CreatePanelImpl(0, tr("Classificator Bar"),
                  QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), SLOT(ShowClassifPanel()));

  ClassifTreeHolder * pCTree = new ClassifTreeHolder(m_Docks[0], m_pDrawWidget, SLOT(Repaint()));
  pCTree->SetRoot(classif().GetMutableRoot());

  m_Docks[0]->setWidget(pCTree);
}

void MainWindow::CreateGuidePanel()
{
  CreatePanelImpl(1, tr("Guide Bar"),
                  QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G), SLOT(ShowGuidePanel()));

  qt::GuidePageHolder * pGPage = new qt::GuidePageHolder(m_Docks[1]);

  m_Docks[1]->setWidget(pGPage);
}

void MainWindow::CreatePanelImpl(size_t i, QString const & name,
                                 QKeySequence const & hotkey, char const * slot)
{
  m_Docks[i] = new QDockWidget(name, this);

  addDockWidget(Qt::LeftDockWidgetArea, m_Docks[i]);

  // hide by default
  m_Docks[i]->hide();

  // register a hotkey to show classificator panel
  QAction * pAct = new QAction(this);
  pAct->setShortcut(hotkey);
  connect(pAct, SIGNAL(triggered()), this, slot);
  addAction(pAct);
}
#endif // DEBUG
}
