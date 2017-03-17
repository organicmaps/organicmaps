#include "search/search_quality/assessment_tool/main_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"
#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/assessment_tool/sample_view.hpp"
#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <QtCore/Qt>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
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

MainView::~MainView()
{
  if (m_framework.GetDrapeEngine() != nullptr)
  {
    m_framework.EnterBackground();
    m_framework.DestroyDrapeEngine();
  }
}

void MainView::SetSamples(std::vector<search::Sample> const & samples)
{
  m_samplesView->SetSamples(samples);
}

void MainView::ShowSample(search::Sample const & sample)
{
  m_framework.ShowRect(sample.m_viewport, -1 /* maxScale */, false /* animation */);

  m_sampleView->SetContents(sample);
  m_sampleView->show();
}

void MainView::ShowResults(search::Results::Iter begin, search::Results::Iter end)
{
  m_sampleView->ShowResults(begin, end);
}

void MainView::SetResultRelevances(
    std::vector<search::Sample::Result::Relevance> const & relevances)
{
  m_sampleView->SetResultRelevances(relevances);
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
  auto * bar = menuBar();

  auto * fileMenu = bar->addMenu(tr("&File"));

  {
    auto * open = new QAction(tr("&Open queries..."), this /* parent */);
    open->setShortcuts(QKeySequence::Open);
    open->setStatusTip(tr("Open the file with queries for assessment"));
    connect(open, &QAction::triggered, this, &MainView::Open);
    fileMenu->addAction(open);
  }

  fileMenu->addSeparator();

  {
    auto * quit = new QAction(tr("&Quit"), this /* parent */);
    quit->setShortcuts(QKeySequence::Quit);
    quit->setStatusTip(tr("Exit the tool"));
    connect(quit, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quit);
  }

  auto * viewMenu = bar->addMenu(tr("&View"));

  {
    CHECK(m_samplesDock != nullptr, ());
    viewMenu->addAction(m_samplesDock->toggleViewAction());
  }

  {
    CHECK(m_sampleDock != nullptr, ());
    viewMenu->addAction(m_sampleDock->toggleViewAction());
  }
}

void MainView::InitMapWidget()
{
  auto * widget = new QWidget(this /* parent */);
  auto * layout = BuildLayoutWithoutMargins<QHBoxLayout>(widget /* parent */);
  widget->setLayout(layout);

  {
    auto * mapWidget = new qt::common::MapWidget(m_framework, widget /* parent */);
    auto * toolBar = new QToolBar(widget /* parent */);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setIconSize(QSize(32, 32));
    qt::common::ScaleSlider::Embed(Qt::Vertical, *toolBar, *mapWidget);

    layout->addWidget(mapWidget);
    layout->addWidget(toolBar);
  }

  setCentralWidget(widget);
}

void MainView::InitDocks()
{
  m_samplesView = new SamplesView(this /* parent */);

  {
    auto * model = m_samplesView->selectionModel();
    connect(model, SIGNAL(selectionChanged(QItemSelection const &, QItemSelection const &)), this,
            SLOT(OnSampleSelected(QItemSelection const &)));
  }

  m_samplesDock = CreateDock("Samples", *m_samplesView);
  addDockWidget(Qt::RightDockWidgetArea, m_samplesDock);

  m_sampleView = new SampleView(this /* parent */);
  m_sampleDock = CreateDock("Sample", *m_sampleView);
  addDockWidget(Qt::RightDockWidgetArea, m_sampleDock);
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

QDockWidget * MainView::CreateDock(std::string const & title, QWidget & widget)
{
  auto * dock = new QDockWidget(ToQString(title), this /* parent */, Qt::Widget);
  dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  dock->setWidget(&widget);
  return dock;
}
