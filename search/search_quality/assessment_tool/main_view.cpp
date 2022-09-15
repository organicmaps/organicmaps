#include "main_view.hpp"

#include "feature_info_dialog.hpp"
#include "helpers.hpp"
#include "model.hpp"
#include "results_view.hpp"
#include "sample_view.hpp"
#include "samples_view.hpp"

#include "qt/qt_common/map_widget.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <limits>

#include <QtCore/Qt>
#include <QtGui/QCloseEvent>
#include <QtGui/QIntValidator>
#include <QtGui/QKeySequence>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolBar>

using Relevance = search::Sample::Result::Relevance;

namespace
{
char const kJSON[] = "JSON Lines files (*.jsonl)";
}  // namespace

MainView::MainView(Framework & framework, QRect const & screenGeometry)
: m_framework(framework)
{
  setGeometry(screenGeometry);

  setWindowTitle(tr("Assessment tool"));
  InitMapWidget();
  InitDocks();
  InitMenuBar();

  m_framework.SetPlacePageListeners(
      [this]() {
        auto const & info = m_framework.GetCurrentPlacePageInfo();
        auto const & selectedFeature = info.GetID();
        if (!selectedFeature.IsValid())
          return;
        m_selectedFeature = selectedFeature;

        if (m_skipFeatureInfoDialog)
        {
          m_skipFeatureInfoDialog = false;
          return;
        }

        auto mapObject = m_framework.GetMapObjectByID(selectedFeature);
        if (!mapObject.GetID().IsValid())
          return;

        auto const address = m_framework.GetAddressAtPoint(mapObject.GetMercator());
        FeatureInfoDialog dialog(this /* parent */, mapObject, address, m_sampleLocale);
        dialog.exec();
      },
      [this](bool /* switchFullScreenMode */) { m_selectedFeature = FeatureID(); },
      {} /* onUpdate */);
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
  m_initiateBackgroundSearch->setEnabled(true);
}

void MainView::OnSearchStarted()
{
  m_state = State::Search;
  m_sampleView->OnSearchStarted();
}

void MainView::OnSearchCompleted()
{
  m_state = State::AfterSearch;
  m_sampleView->OnSearchCompleted();
}

void MainView::ShowSample(size_t sampleIndex, search::Sample const & sample,
                          std::optional<m2::PointD> const & position, bool isUseless, bool hasEdits)
{
  m_sampleLocale = sample.m_locale;

  MoveViewportToRect(sample.m_viewport);

  m_sampleView->SetContents(sample, position);
  m_sampleView->show();

  OnResultChanged(sampleIndex, ResultType::Found, ResultsEdits::Update::MakeAll());
  OnResultChanged(sampleIndex, ResultType::NonFound, ResultsEdits::Update::MakeAll());
  OnSampleChanged(sampleIndex, isUseless, hasEdits);
}

void MainView::AddFoundResults(search::Results const & results)
{
  m_sampleView->AddFoundResults(results);
}

void MainView::ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                                   std::vector<ResultsEdits::Entry> const & entries)
{
  m_sampleView->ShowNonFoundResults(results, entries);
}

void MainView::ShowMarks(Context const & context)
{
  m_sampleView->ClearSearchResultMarks();
  m_sampleView->ShowFoundResultsMarks(context.m_foundResults);
  m_sampleView->ShowNonFoundResultsMarks(context.m_nonFoundResults, context.m_nonFoundResultsEdits.GetEntries());
}

void MainView::MoveViewportToResult(search::Result const & result)
{
  m_skipFeatureInfoDialog = true;
  m_framework.SelectSearchResult(result, false /* animation */);
}

void MainView::MoveViewportToResult(search::Sample::Result const & result)
{
  int constexpr kViewportAroundResultSizeM = 100;
  auto const rect =
      mercator::RectByCenterXYAndSizeInMeters(result.m_pos, kViewportAroundResultSizeM);
  MoveViewportToRect(rect);
}

void MainView::MoveViewportToRect(m2::RectD const & rect)
{
  m_framework.ShowRect(rect, -1 /* maxScale */, false /* animation */);
}

void MainView::OnResultChanged(size_t sampleIndex, ResultType type,
                               ResultsEdits::Update const & update)
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

void MainView::OnSampleChanged(size_t sampleIndex, bool isUseless, bool hasEdits)
{
  m_samplesView->OnUpdate(sampleIndex);
  if (!m_samplesView->IsSelected(sampleIndex))
    return;
  SetSampleDockTitle(isUseless, hasEdits);
  m_sampleView->OnUselessnessChanged(isUseless);
}

void MainView::OnSamplesChanged(bool hasEdits)
{
  SetSamplesDockTitle(hasEdits);
  m_save->setEnabled(hasEdits);
  m_saveAs->setEnabled(hasEdits);
}

void MainView::SetResultsEdits(size_t sampleIndex, ResultsEdits & foundResultsEdits,
                               ResultsEdits & nonFoundResultsEdits)
{
  CHECK(m_samplesView->IsSelected(sampleIndex), ());
  m_sampleView->SetResultsEdits(foundResultsEdits, nonFoundResultsEdits);
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
  SetSampleDockTitle(false /* isUseless */, false /* hasEdits */);

  m_skipFeatureInfoDialog = false;
  m_sampleLocale.clear();
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

  {
    m_initiateBackgroundSearch = new QAction(tr("Initiate background search"), this /* parent */);
    m_initiateBackgroundSearch->setShortcut(Qt::CTRL | Qt::Key_I);
    m_initiateBackgroundSearch->setStatusTip(
        tr("Search in the background for the queries from a selected range"));
    m_initiateBackgroundSearch->setEnabled(false);
    connect(m_initiateBackgroundSearch, &QAction::triggered, this,
            &MainView::InitiateBackgroundSearch);
    fileMenu->addAction(m_initiateBackgroundSearch);
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
    auto * mapWidget = new qt::common::MapWidget(m_framework, false /* apiOpenGLES3 */,
                                                 false /* screenshotMode */, widget /* parent */);
    connect(mapWidget, &qt::common::MapWidget::OnContextMenuRequested,
            [this](QPoint const & p) { AddSelectedFeature(p); });
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

  connect(m_samplesView, &SamplesView::FlipSampleUsefulness,
          [this](int index) { m_model->FlipSampleUsefulness(index); });

  m_samplesDock = CreateDock(*m_samplesView);
  addDockWidget(Qt::LeftDockWidgetArea, m_samplesDock);
  SetSamplesDockTitle(false /* hasEdits */);

  m_sampleView = new SampleView(this /* parent */, m_framework);

  connect(m_sampleView, &SampleView::OnShowViewportClicked,
          [this]() { m_model->OnShowViewportClicked(); });
  connect(m_sampleView, &SampleView::OnShowPositionClicked,
          [this]() { m_model->OnShowPositionClicked(); });

  connect(m_sampleView, &SampleView::OnMarkAllAsRelevantClicked,
          [this]() { m_model->OnMarkAllAsRelevantClicked(); });
  connect(m_sampleView, &SampleView::OnMarkAllAsIrrelevantClicked,
          [this]() { m_model->OnMarkAllAsIrrelevantClicked(); });

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
  connect(m_sampleDock, &QDockWidget::dockLocationChanged,
          [this](Qt::DockWidgetArea area) { m_sampleView->OnLocationChanged(area); });
  addDockWidget(Qt::RightDockWidgetArea, m_sampleDock);
  SetSampleDockTitle(false /* isUseless */, false /* hasEdits */);
}

void MainView::Open()
{
  CHECK(m_model, ());

  if (TryToSaveEdits(tr("Save changes before opening samples?")) == SaveResult::Cancelled)
    return;

  auto const name = QFileDialog::getOpenFileName(this /* parent */, tr("Open samples..."),
                                                 QString() /* dir */, kJSON);
  auto const file = name.toStdString();
  if (file.empty())
    return;

  m_model->Open(file);
}

void MainView::Save() { m_model->Save(); }

void MainView::SaveAs()
{
  auto const name = QFileDialog::getSaveFileName(this /* parent */, tr("Save samples as..."),
                                                 QString() /* dir */, kJSON);
  auto const file = name.toStdString();
  if (!file.empty())
    m_model->SaveAs(file);
}

void MainView::InitiateBackgroundSearch()
{
  QDialog dialog(this);
  QFormLayout form(&dialog);

  form.addRow(new QLabel("Queries range"));

  QValidator * validator = new QIntValidator(0, std::numeric_limits<int>::max(), this);

  QLineEdit * lineEditFrom = new QLineEdit(&dialog);
  form.addRow(new QLabel("First"), lineEditFrom);
  lineEditFrom->setValidator(validator);

  QLineEdit * lineEditTo = new QLineEdit(&dialog);
  form.addRow(new QLabel("Last"), lineEditTo);
  lineEditTo->setValidator(validator);

  QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal,
                             &dialog);
  form.addRow(&buttonBox);

  connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
    return;

  std::string const strFrom = lineEditFrom->text().toStdString();
  std::string const strTo = lineEditTo->text().toStdString();
  uint64_t from = 0;
  uint64_t to = 0;
  if (!strings::to_uint64(strFrom, from))
  {
    LOG(LERROR, ("Could not parse number from", strFrom));
    return;
  }
  if (!strings::to_uint64(strTo, to))
  {
    LOG(LERROR, ("Could not parse number from", strTo));
    return;
  }

  m_model->InitiateBackgroundSearch(base::checked_cast<size_t>(from),
                                    base::checked_cast<size_t>(to));
}

void MainView::SetSamplesDockTitle(bool hasEdits)
{
  CHECK(m_samplesDock, ());
  if (hasEdits)
    m_samplesDock->setWindowTitle(tr("Samples *"));
  else
    m_samplesDock->setWindowTitle(tr("Samples"));
}

void MainView::SetSampleDockTitle(bool isUseless, bool hasEdits)
{
  CHECK(m_sampleDock, ());
  std::string title = "Sample";
  if (hasEdits)
    title += " *";
  if (isUseless)
    title += " (useless)";
  m_sampleDock->setWindowTitle(tr(title.data()));
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

void MainView::AddSelectedFeature(QPoint const & p)
{
  auto const selectedFeature = m_selectedFeature;

  if (!selectedFeature.IsValid())
    return;

  if (m_state != State::AfterSearch)
    return;

  if (m_model->AlreadyInSamples(selectedFeature))
    return;

  QMenu menu;
  auto const * action = menu.addAction("Add to non-found results");
  connect(action, &QAction::triggered,
          [this, selectedFeature]() { m_model->AddNonFoundResult(selectedFeature); });
  menu.exec(p);
}

QDockWidget * MainView::CreateDock(QWidget & widget)
{
  auto * dock = new QDockWidget(QString(), this /* parent */, Qt::Widget);
  dock->setFeatures(QDockWidget::DockWidgetClosable |
                    QDockWidget::DockWidgetMovable |
                    QDockWidget::DockWidgetFloatable);
  dock->setWidget(&widget);
  return dock;
}
