#include "search/search_quality/assessment_tool/main_view.hpp"

#include "search/search_quality/assessment_tool/main_model.hpp"

#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <QtCore/QList>
#include <QtCore/Qt>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolBar>

MainView::MainView(Framework & framework) : m_framework(framework)
{
  setWindowTitle(tr("Assessment tool"));
  InitMapWidget();
  InitDocks();
  InitMenuBar();
}

void MainView::SetSamples(std::vector<search::Sample> const & samples)
{
  m_samplesModel->removeRows(0, m_samplesModel->rowCount());
  for (auto const & sample : samples)
  {
    m_samplesModel->appendRow(
        new QStandardItem(QString::fromUtf8(strings::ToUtf8(sample.m_query).c_str())));
  }
}

void MainView::ShowSample(search::Sample const & sample)
{
  // TODO (@y): implement a dock view for search sample.
}

void MainView::ShowError(std::string const & msg)
{
  QMessageBox box(QMessageBox::Critical /* icon */, tr("Error") /* title */,
                  QString::fromStdString(msg) /* text */, QMessageBox::Ok /* buttons */,
                  this /* parent */);
  box.exec();
}

void MainView::OnSampleSelected(QItemSelection const & current)
{
  CHECK(m_model, ());
  auto indexes = current.indexes();
  for (auto const & index : indexes)
    m_model->OnSampleSelected(index.row());
}

void MainView::InitMenuBar()
{
  CHECK(m_samplesDock.get(), ());

  auto * bar = menuBar();

  auto * fileMenu = bar->addMenu(tr("&File"));

  {
    auto open = make_unique<QAction>(tr("&Open queries..."), this);
    open->setShortcuts(QKeySequence::Open);
    open->setStatusTip(tr("Open the file with queries for assessment"));
    connect(open.get(), &QAction::triggered, this, &MainView::Open);
    fileMenu->addAction(open.release());
  }

  fileMenu->addSeparator();

  {
    auto quit = make_unique<QAction>(tr("&Quit"), this);
    quit->setShortcuts(QKeySequence::Quit);
    quit->setStatusTip(tr("Exit the tool"));
    connect(quit.get(), &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quit.release());
  }

  auto * viewMenu = bar->addMenu(tr("&View"));
  {
    viewMenu->addAction(m_samplesDock->toggleViewAction());
  }
}

void MainView::InitMapWidget()
{
  auto widget = make_unique<QWidget>(this /* parent */);

  auto layout = make_unique<QHBoxLayout>(widget.get() /* parent */);
  layout->setContentsMargins(0 /* left */, 0 /* top */, 0 /* right */, 0 /* bottom */);
  {
    auto mapWidget = make_unique<qt::common::MapWidget>(m_framework, widget.get() /* parent */);
    auto toolBar = make_unique<QToolBar>(widget.get() /* parent */);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setIconSize(QSize(32, 32));
    qt::common::ScaleSlider::Embed(Qt::Vertical, *toolBar, *mapWidget);

    layout->addWidget(mapWidget.release());
    layout->addWidget(toolBar.release());
  }

  widget->setLayout(layout.release());
  setCentralWidget(widget.release());
}

void MainView::InitDocks()
{
  m_samplesTable = my::make_unique<QTableView>(this /* parent */);
  m_samplesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_samplesTable->setSelectionMode(QAbstractItemView::SingleSelection);

  {
    auto * header = m_samplesTable->horizontalHeader();
    header->setStretchLastSection(true /* stretch */);
    header->hide();
  }

  m_samplesModel =
      my::make_unique<QStandardItemModel>(0 /* rows */, 1 /* columns */, this /* parent */);

  m_samplesTable->setModel(m_samplesModel.get());

  {
    auto * model = m_samplesTable->selectionModel();
    connect(model, SIGNAL(selectionChanged(QItemSelection const &, QItemSelection const &)), this,
            SLOT(OnSampleSelected(QItemSelection const &)));
  }

  m_samplesDock = my::make_unique<QDockWidget>(tr("Samples"), this /* parent */, Qt::Widget);
  m_samplesDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  m_samplesDock->setWidget(m_samplesTable.get());
  addDockWidget(Qt::RightDockWidgetArea, m_samplesDock.get());
}

void MainView::Open()
{
  CHECK(m_model, ());

  auto const file = QFileDialog::getOpenFileName(this, tr("Open samples..."), QString(),
                                                 tr("JSON files (*.json)"))
                        .toStdString();
  if (file.empty())
    return;

  m_model->Open(file);
}
