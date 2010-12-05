#include "../base/SRC_FIRST.hpp"

#include "mainwindow.hpp"
#include "draw_widget.hpp"
#include "update_dialog.hpp"
#include "searchwindow.hpp"
#include "classificator_tree.hpp"

#include "../indexer/classificator.hpp"

#include "../platform/platform.hpp"

#include <QtGui/QDockWidget>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QSlider>

#include "../base/start_mem_debug.hpp"

namespace qt
{

MainWindow::MainWindow()
{
  m_pDrawWidget = new DrawWidget(this, m_Storage);

  CreateNavigationBar();

  CreateClassifPanel();

  setCentralWidget(m_pDrawWidget);

  setWindowTitle(tr("MapsWithMe"));

  LoadState();
}

MainWindow::~MainWindow()
{
  SaveState();

  GetDownloadManager().CancelAllDownloads();
}

string MainWindow::GetIniFile()
{
  return GetPlatform().WorkingDir() + "settings.ini";
}

void MainWindow::SaveState()
{
  try
  {
    FileWriter writer(GetIniFile());
    stream::SinkWriterStream<FileWriter> ss(writer);

    ss << (uint32_t)x();
    ss << (uint32_t)y();
    ss << (uint32_t)width();
    ss << (uint32_t)height();

    m_pDrawWidget->SaveState(writer);
  }
  catch (Writer::Exception const &)
  {
  }
}

void MainWindow::LoadState()
{
  try
  {
    FileReader reader(GetIniFile());
    ReaderSource<FileReader> src(reader);

    stream::SinkReaderStream<ReaderSource<FileReader> > ss(src);

    uint32_t x, y, width, height;
    ss >> x >> y >> width >> height;
    move(x, y);
    resize(width, height);

    m_pDrawWidget->LoadState(src);
  }
  catch(Reader::Exception const &)
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
  QSlider * pScale = new QSlider(Qt::Vertical, this);
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
  UpdateDialog dlg(this, m_Storage);
  dlg.exec();
}

void MainWindow::ShowClassifPanel()
{
  m_pClassifDock->show();
}

}
