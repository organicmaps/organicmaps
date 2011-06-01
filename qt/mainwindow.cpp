#include "mainwindow.hpp"
#include "draw_widget.hpp"
#include "slider_ctrl.hpp"
#include "about.hpp"
#include "preferences_dialog.hpp"

#include "../defines.hpp"

#include "../search/result.hpp"

#include "../map/settings.hpp"

#include "../std/bind.hpp"

#include <QtGui/QDockWidget>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtGui/QLineEdit>
#include <QtGui/QHeaderView>
#include <QtGui/QTableWidget>

#define IDM_ABOUT_DIALOG        1001
#define IDM_PREFERENCES_DIALOG  1002

#ifndef NO_DOWNLOADER
#include "update_dialog.hpp"
#include "classificator_tree.hpp"
#include "info_dialog.hpp"
#include "guide_page.hpp"

#include "../indexer/classificator.hpp"

#include <QtCore/QFile>

#endif // NO_DOWNLOADER

namespace qt
{

MainWindow::MainWindow()
#ifndef NO_DOWNLOADER
  : m_updateDialog(0)
#endif // NO_DOWNLOADER
{
  m_pDrawWidget = new DrawWidget(this, m_storage);

  CreateNavigationBar();

  CreateSearchBarAndPanel();

#ifndef NO_DOWNLOADER
  CreateClassifPanel();
  CreateGuidePanel();
#endif // NO_DOWNLOADER
  setCentralWidget(m_pDrawWidget);

  setWindowTitle(tr("MapsWithMe"));
  setWindowIcon(QIcon(":logo.png"));

#ifndef OMIM_OS_WINDOWS
  QMenu * helpMenu = new QMenu(tr("Help"), this);
  menuBar()->addMenu(helpMenu);
  helpMenu->addAction(tr("About"), this, SLOT(OnAbout()));
  helpMenu->addAction(tr("Preferences"), this, SLOT(OnPreferences()));
#else
  {
    // create items in the system menu
    HMENU menu = ::GetSystemMenu(winId(), FALSE);
    MENUITEMINFOA item;
    item.cbSize = sizeof(MENUITEMINFOA);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    item.fType = MFT_STRING;
    item.wID = IDM_PREFERENCES_DIALOG;
    QByteArray const prefsStr = tr("Preferences...").toLocal8Bit();
    item.dwTypeData = const_cast<char *>(prefsStr.data());
    item.cch = prefsStr.size();
    ::InsertMenuItemA(menu, ::GetMenuItemCount(menu) - 1, TRUE, &item);
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

#ifndef NO_DOWNLOADER
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
#endif // NO_DOWNLOADER
}

#if defined(Q_WS_WIN)
bool MainWindow::winEvent(MSG * msg, long * result)
{
  if (msg->message == WM_SYSCOMMAND)
  {
    switch (msg->wParam)
    {
    case IDM_PREFERENCES_DIALOG:
      OnPreferences();
      *result = 0;
      return true;
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
  QToolBar * pToolBar = new QToolBar(tr("Navigation Bar"), this);
  pToolBar->setOrientation(Qt::Vertical);
  pToolBar->setIconSize(QSize(32, 32));

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
    // add search button with "checked" behavior
    m_pSearchAction = pToolBar->addAction(QIcon(":/navig64/search.png"),
                                           tr("Search"),
                                           this,
                                           SLOT(OnSearchButtonClicked()));
    m_pSearchAction->setCheckable(true);
    m_pSearchAction->setToolTip(tr("Offline Search"));

    pToolBar->addSeparator();

    // add my position button with "checked" behavior
    m_pMyPositionAction = pToolBar->addAction(QIcon(":/navig64/location.png"),
                                           tr("My Position"),
                                           this,
                                           SLOT(OnMyPosition()));
    m_pMyPositionAction->setCheckable(true);
    m_pMyPositionAction->setToolTip(tr("My Position"));

    // add view actions 1
    button_t arr[] = {
      { QString(), 0, 0 },
      { tr("Show all"), ":/navig64/world.png", SLOT(ShowAll()) },
      { tr("Scale +"), ":/navig64/plus.png", SLOT(ScalePlus()) }
    };
    add_buttons(pToolBar, arr, ARRAY_SIZE(arr), m_pDrawWidget);
  }

  // add scale slider
  QClickSlider * pScale = new QClickSlider(Qt::Vertical, this);
  pScale->setRange(3, scales::GetUpperScale());
  pScale->setTickPosition(QSlider::TicksRight);

  pToolBar->addWidget(pScale);
  m_pDrawWidget->SetScaleControl(pScale);

  {
    // add view actions 2
    button_t arr[] = {
      { tr("Scale -"), ":/navig64/minus.png", SLOT(ScaleMinus()) }
    };
    add_buttons(pToolBar, arr, ARRAY_SIZE(arr), m_pDrawWidget);
  }
#ifndef NO_DOWNLOADER
  {
    // add mainframe actions
    button_t arr[] = {
      { QString(), 0, 0 },
      { tr("Download Maps"), ":/navig64/download.png", SLOT(ShowUpdateDialog()) }
    };
    add_buttons(pToolBar, arr, ARRAY_SIZE(arr), this);
  }
#endif // NO_DOWNLOADER
  addToolBar(Qt::RightToolBarArea, pToolBar);
}

void MainWindow::OnAbout()
{
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::OnLocationFound()
{
  m_pMyPositionAction->setIcon(QIcon(":/navig64/location.png"));
  m_pMyPositionAction->setToolTip(tr("My Position"));
}

void MainWindow::OnMyPosition()
{
  if (m_pMyPositionAction->isChecked())
  {
    m_pMyPositionAction->setIcon(QIcon(":/navig64/location-search.png"));
    m_pMyPositionAction->setToolTip(tr("Looking for position..."));
    m_pDrawWidget->OnEnableMyPosition(bind(&MainWindow::OnLocationFound, this));
  }
  else
  {
    m_pMyPositionAction->setIcon(QIcon(":/navig64/location.png"));
    m_pMyPositionAction->setToolTip(tr("My Position"));
    m_pDrawWidget->OnDisableMyPosition();
  }
}

void MainWindow::OnSearchButtonClicked()
{
  if (m_pSearchAction->isChecked())
  {
    m_Docks[2]->show();
    m_Docks[2]->widget()->setFocus();
  }
  else
  {
    m_Docks[2]->hide();
  }
}

void MainWindow::OnSearchShortcutPressed()
{
  if (m_Docks[2]->isVisible())
  {
    m_pSearchAction->setChecked(false);
    m_Docks[2]->hide();
  }
  else
  {
    m_pSearchAction->setChecked(true);
    m_Docks[2]->show();
    m_Docks[2]->widget()->setFocus();
  }
}

void MainWindow::OnSearchTextChanged(QString const & str)
{
  // clear old results
  QTableWidget * table = static_cast<QTableWidget *>(m_Docks[3]->widget());
  table->clear();
  table->setRowCount(0);
  QString const normalized = str.normalized(QString::NormalizationForm_KC);
  if (!normalized.isEmpty())
    m_pDrawWidget->Search(normalized.toUtf8().constData(),
                        bind(&MainWindow::OnSearchResult, this, _1));
}

void MainWindow::OnSearchResult(search::Result const & result)
{
  if (result.GetString().empty())  // last element
  {
    if (!m_Docks[3]->isVisible())
      m_Docks[3]->show();
  }
  else
  {
    QTableWidget * table = static_cast<QTableWidget *>(m_Docks[3]->widget());

    int const rowCount = table->rowCount();

    table->setRowCount(rowCount + 1);
    QTableWidgetItem * item = new QTableWidgetItem(QString::fromUtf8(result.GetString().c_str()));
    item->setData(Qt::UserRole, QRectF(QPointF(result.GetRect().minX(), result.GetRect().maxY()),
                                       QPointF(result.GetRect().maxX(), result.GetRect().minY())));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->setItem(rowCount, 0, item);
  }
}

void MainWindow::OnSearchPanelShortcutPressed()
{
  if (m_Docks[3]->isVisible())
    m_Docks[3]->hide();
  else
    m_Docks[3]->show();
}

void MainWindow::OnSearchPanelItemClicked(int row, int)
{
  // center viewport on clicked item
  QTableWidget * table = static_cast<QTableWidget *>(m_Docks[3]->widget());
  QRectF rect = table->item(row, 0)->data(Qt::UserRole).toRectF();
  m2::RectD r2(rect.left(), rect.bottom(), rect.right(), rect.top());
//  r2.Inflate(0.0001, 0.0001);
  m_pDrawWidget->ShowFeature(r2);
}

void MainWindow::OnPreferences()
{
  bool autoUpdatesEnabled = DEFAULT_AUTO_UPDATES_ENABLED;
  Settings::Get("AutomaticUpdateCheck", autoUpdatesEnabled);

  PreferencesDialog dlg(this, autoUpdatesEnabled);
  dlg.exec();

  Settings::Set("AutomaticUpdateCheck", autoUpdatesEnabled);
}

#ifndef NO_DOWNLOADER
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

void MainWindow::CreateClassifPanel()
{
  CreatePanelImpl(0, Qt::LeftDockWidgetArea, tr("Classificator Bar"),
                  QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), SLOT(ShowClassifPanel()));

  ClassifTreeHolder * pCTree = new ClassifTreeHolder(m_Docks[0], m_pDrawWidget, SLOT(Repaint()));
  pCTree->SetRoot(classif().GetMutableRoot());

  m_Docks[0]->setWidget(pCTree);
}

void MainWindow::CreateGuidePanel()
{
  CreatePanelImpl(1, Qt::LeftDockWidgetArea, tr("Guide Bar"),
                  QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G), SLOT(ShowGuidePanel()));

  qt::GuidePageHolder * pGPage = new qt::GuidePageHolder(m_Docks[1]);

  m_Docks[1]->setWidget(pGPage);
}
#endif // NO_DOWNLOADER

void MainWindow::CreateSearchBarAndPanel()
{
  CreatePanelImpl(2, Qt::TopDockWidgetArea, tr("Search Bar"),
                  QKeySequence(Qt::CTRL + Qt::Key_F), SLOT(OnSearchShortcutPressed()));

  QLineEdit * editor = new QLineEdit(m_Docks[2]);
  connect(editor, SIGNAL(textChanged(QString const &)), this, SLOT(OnSearchTextChanged(QString const &)));

  m_Docks[2]->setFeatures(QDockWidget::NoDockWidgetFeatures);
  // @TODO remove search bar title
  m_Docks[2]->setWidget(editor);

  // also create search results panel
  CreatePanelImpl(3, Qt::LeftDockWidgetArea, tr("Search Results"),
                  QKeySequence(Qt::CTRL + Qt::ShiftModifier + Qt::Key_F),
                  SLOT(OnSearchPanelShortcutPressed()));

  QTableWidget * panel = new QTableWidget(0, 2, m_Docks[3]);
  panel->setAlternatingRowColors(true);
  panel->setShowGrid(false);
  panel->setSelectionBehavior(QAbstractItemView::SelectRows);
  panel->verticalHeader()->setVisible(false);
  panel->horizontalHeader()->setVisible(false);
  panel->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

  connect(panel, SIGNAL(cellClicked(int,int)), this, SLOT(OnSearchPanelItemClicked(int,int)));
  m_Docks[3]->setWidget(panel);
}

void MainWindow::CreatePanelImpl(size_t i, Qt::DockWidgetArea area, QString const & name,
                                 QKeySequence const & hotkey, char const * slot)
{
  m_Docks[i] = new QDockWidget(name, this);

  addDockWidget(area, m_Docks[i]);

  // hide by default
  m_Docks[i]->hide();

  // register a hotkey to show classificator panel
  QAction * pAct = new QAction(this);
  pAct->setShortcut(hotkey);
  connect(pAct, SIGNAL(triggered()), this, slot);
  addAction(pAct);
}

}
