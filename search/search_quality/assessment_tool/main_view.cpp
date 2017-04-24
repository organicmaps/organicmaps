#include "search/search_quality/assessment_tool/main_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"
#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/assessment_tool/results_view.hpp"
#include "search/search_quality/assessment_tool/sample_view.hpp"
#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <QtCore/Qt>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolBar>

using Relevance = search::Sample::Result::Relevance;

namespace
{
char const kJSON[] = "JSON files (*.json)";
}  // namespace

MainView::MainView(Framework & framework) : m_framework(framework)
{
  QDesktopWidget const * desktop = QApplication::desktop();
  setGeometry(desktop->screenGeometry(desktop->primaryScreen()));

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

void MainView::SetSamples(ContextList::SamplesSlice const & samples)
{
  m_samplesView->SetSamples(samples);
  m_sampleView->Clear();
}

void MainView::ShowSample(size_t sampleIndex, search::Sample const & sample, bool positionAvailable,
                          bool hasEdits)
{
  MoveViewportToRect(sample.m_viewport);

  m_sampleView->SetContents(sample, positionAvailable);
  m_sampleView->show();

  OnResultChanged(sampleIndex, ResultType::Found, Edits::Update::AllRelevancesUpdate());
  OnResultChanged(sampleIndex, ResultType::NonFound, Edits::Update::AllRelevancesUpdate());
  OnSampleChanged(sampleIndex, hasEdits);
}

void MainView::ShowFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end)
{
  m_sampleView->ShowFoundResults(begin, end);
}

void MainView::ShowNonFoundResults(std::vector<search::Sample::Result> const & results)
{
  m_sampleView->ShowNonFoundResults(results);
}

void MainView::MoveViewportToResult(search::Result const & result)
{
  m_framework.ShowSearchResult(result, false /* animation */);
}

void MainView::MoveViewportToResult(search::Sample::Result const & result)
{
  int constexpr kViewportAroundResultSizeM = 100;
  auto const rect =
      MercatorBounds::RectByCenterXYAndSizeInMeters(result.m_pos, kViewportAroundResultSizeM);
  MoveViewportToRect(rect);
}

void MainView::MoveViewportToRect(m2::RectD const & rect)
{
  m_framework.ShowRect(rect, -1 /* maxScale */, false /* animation */);
}

void MainView::OnResultChanged(size_t sampleIndex, ResultType type, Edits::Update const & update)
{
  m_samplesView->OnUpdate(sampleIndex);

  if (!m_samplesView->IsSelected(sampleIndex))
    return;
  switch (type)
  {
  case ResultType::Found: m_sampleView->GetFoundResultsView().Update(update); break;
  case ResultType::NonFound: m_sampleView->GetNonFoundResultsView().Update(update); break;
  }
}

void MainView::OnSampleChanged(size_t sampleIndex, bool hasEdits)
{
  if (!m_samplesView->IsSelected(sampleIndex))
    return;
  SetSampleDockTitle(hasEdits);
}

void MainView::OnSamplesChanged(bool hasEdits)
{
  SetSamplesDockTitle(hasEdits);
  m_save->setEnabled(hasEdits);
  m_saveAs->setEnabled(hasEdits);
}

void MainView::EnableSampleEditing(size_t sampleIndex, Edits & foundResultsEdits,
                                   Edits & nonFoundResultsEdits)
{
  CHECK(m_samplesView->IsSelected(sampleIndex), ());
  m_sampleView->EnableEditing(foundResultsEdits, nonFoundResultsEdits);
}

void MainView::ShowError(std::string const & msg)
{
  QMessageBox box(QMessageBox::Critical /* icon */, tr("Error") /* title */,
                  QString::fromStdString(msg) /* text */, QMessageBox::Ok /* buttons */,
                  this /* parent */);
  box.exec();
}

void MainView::Clear()
{
  m_samplesView->Clear();
  SetSamplesDockTitle(false /* hasEdits */);

  m_sampleView->Clear();
  SetSampleDockTitle(false /* hasEdits */);
}

void MainView::closeEvent(QCloseEvent * event)
{
  if (TryToSaveEdits(tr("Save changes before closing?")) == SaveResult::Cancelled)
    event->ignore();
  else
    event->accept();
}

void MainView::OnSampleSelected(QItemSelection const & current)
{
  CHECK(m_model, ());
  auto const indexes = current.indexes();
  for (auto const & index : indexes)
    m_model->OnSampleSelected(index.row());
}

void MainView::OnResultSelected(QItemSelection const & current)
{
  CHECK(m_model, ());
  auto const indexes = current.indexes();
  for (auto const & index : indexes)
    m_model->OnResultSelected(index.row());
}

void MainView::OnNonFoundResultSelected(QItemSelection const & current)
{
  CHECK(m_model, ());
  auto const indexes = current.indexes();
  for (auto const & index : indexes)
    m_model->OnNonFoundResultSelected(index.row());
}

void MainView::InitMenuBar()
{
  auto * bar = menuBar();

  auto * fileMenu = bar->addMenu(tr("&File"));

  {
    auto * open = new QAction(tr("&Open samples..."), this /* parent */);
    open->setShortcuts(QKeySequence::Open);
    open->setStatusTip(tr("Open the file with samples for assessment"));
    connect(open, &QAction::triggered, this, &MainView::Open);
    fileMenu->addAction(open);
  }

  {
    m_save = new QAction(tr("Save samples"), this /* parent */);
    m_save->setShortcuts(QKeySequence::Save);
    m_save->setStatusTip(tr("Save the file with assessed samples"));
    m_save->setEnabled(false);
    connect(m_save, &QAction::triggered, this, &MainView::Save);
    fileMenu->addAction(m_save);
  }

  {
    m_saveAs = new QAction(tr("Save samples as..."), this /* parent */);
    m_saveAs->setShortcuts(QKeySequence::SaveAs);
    m_saveAs->setStatusTip(tr("Save the file with assessed samples"));
    m_saveAs->setEnabled(false);
    connect(m_saveAs, &QAction::triggered, this, &MainView::SaveAs);
    fileMenu->addAction(m_saveAs);
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
    connect(model, &QItemSelectionModel::selectionChanged, this, &MainView::OnSampleSelected);
  }

  m_samplesDock = CreateDock(*m_samplesView);
  addDockWidget(Qt::RightDockWidgetArea, m_samplesDock);
  SetSamplesDockTitle(false /* hasEdits */);

  m_sampleView = new SampleView(this /* parent */);

  connect(m_sampleView, &SampleView::OnShowViewportClicked,
          [this]() { m_model->OnShowViewportClicked(); });
  connect(m_sampleView, &SampleView::OnShowPositionClicked,
          [this]() { m_model->OnShowPositionClicked(); });

  {
    auto const & view = m_sampleView->GetFoundResultsView();
    connect(&view, &ResultsView::OnResultSelected,
            [this](int index) { m_model->OnResultSelected(index); });
  }

  {
    auto const & view = m_sampleView->GetNonFoundResultsView();
    connect(&view, &ResultsView::OnResultSelected,
            [this](int index) { m_model->OnNonFoundResultSelected(index); });
  }

  m_sampleDock = CreateDock(*m_sampleView);
  addDockWidget(Qt::RightDockWidgetArea, m_sampleDock);
  SetSampleDockTitle(false /* hasEdits */);
}

void MainView::Open()
{
  CHECK(m_model, ());

  if (TryToSaveEdits(tr("Save changes before opening samples?")) == SaveResult::Cancelled)
    return;

  auto const name = QFileDialog::getOpenFileName(this /* parent */, tr("Open samples..."),
                                                 QString() /* dir */, tr(kJSON));
  auto const file = name.toStdString();
  if (file.empty())
    return;

  m_model->Open(file);
}

void MainView::Save() { m_model->Save(); }

void MainView::SaveAs()
{
  auto const name = QFileDialog::getSaveFileName(this /* parent */, tr("Save samples as..."),
                                                 QString() /* dir */, tr(kJSON));
  auto const file = name.toStdString();
  if (!file.empty())
    m_model->SaveAs(file);
}

void MainView::SetSamplesDockTitle(bool hasEdits)
{
  CHECK(m_samplesDock, ());
  if (hasEdits)
    m_samplesDock->setWindowTitle(tr("Samples *"));
  else
    m_samplesDock->setWindowTitle(tr("Samples"));
}

void MainView::SetSampleDockTitle(bool hasEdits)
{
  CHECK(m_sampleDock, ());
  if (hasEdits)
    m_sampleDock->setWindowTitle(tr("Sample *"));
  else
    m_sampleDock->setWindowTitle(tr("Sample"));
}

MainView::SaveResult MainView::TryToSaveEdits(QString const & msg)
{
  CHECK(m_model, ());

  if (!m_model->HasChanges())
    return SaveResult::NoEdits;

  QMessageBox box(QMessageBox::Question /* icon */, tr("Save edits?") /* title */, msg /* text */,
                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel /* buttons */,
                  this /* parent */);
  auto const button = box.exec();
  switch (button)
  {
  case QMessageBox::Save: Save(); return SaveResult::Saved;
  case QMessageBox::Discard: return SaveResult::Discarded;
  case QMessageBox::Cancel: return SaveResult::Cancelled;
  }

  CHECK(false, ());
  return SaveResult::Cancelled;
}

QDockWidget * MainView::CreateDock(QWidget & widget)
{
  auto * dock = new QDockWidget(QString(), this /* parent */, Qt::Widget);
  dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  dock->setWidget(&widget);
  return dock;
}
