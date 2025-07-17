#include "sample_view.hpp"

#include "helpers.hpp"
#include "result_view.hpp"
#include "results_view.hpp"

#include "qt/qt_common/spinner.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/search_mark.hpp"

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace
{
template <typename Layout>
Layout * BuildSubLayout(QLayout & mainLayout, QWidget & parent, QWidget ** box)
{
  *box = new QWidget(&parent);
  auto * subLayout = BuildLayoutWithoutMargins<Layout>(*box /* parent */);
  (*box)->setLayout(subLayout);
  mainLayout.addWidget(*box);
  return subLayout;
}

template <typename Layout>
Layout * BuildSubLayout(QLayout & mainLayout, QWidget & parent)
{
  QWidget * box = nullptr;
  return BuildSubLayout<Layout>(mainLayout, parent, &box);
}

void SetVerticalStretch(QWidget & widget, int stretch)
{
  auto policy = widget.sizePolicy();
  policy.setVerticalStretch(stretch);
  widget.setSizePolicy(policy);
}
}  // namespace

SampleView::SampleView(QWidget * parent, Framework & framework)
  : QWidget(parent), m_framework(framework)
{
  m_framework.GetBookmarkManager().GetEditSession().SetIsVisible(UserMark::Type::SEARCH, true);
  m_framework.GetBookmarkManager().GetEditSession().SetIsVisible(UserMark::Type::COLORED, true);

  auto * mainLayout = BuildLayout<QVBoxLayout>(this /* parent */);

  // When the dock for SampleView is attached to the right side of the
  // screen, we don't need left margin, because of zoom in/zoom out
  // slider. In other cases, it's better to keep left margin as is.
  m_defaultMargins = mainLayout->contentsMargins();
  m_rightAreaMargins = m_defaultMargins;
  m_rightAreaMargins.setLeft(0);

  {
    m_query = new QLabel(this /* parent */);
    m_query->setToolTip(tr("Query text"));
    m_query->setWordWrap(true);
    m_query->hide();
    mainLayout->addWidget(m_query);
  }

  {
    m_langs = new QLabel(this /* parent */);
    m_langs->setToolTip(tr("Query input language"));
    m_langs->hide();
    mainLayout->addWidget(m_langs);
  }

  {
    auto * layout = BuildSubLayout<QHBoxLayout>(*mainLayout, *this /* parent */);

    m_showViewport = new QPushButton(tr("Show viewport"), this /* parent */);
    connect(m_showViewport, &QPushButton::clicked, [this]() { emit OnShowViewportClicked(); });
    layout->addWidget(m_showViewport);

    m_showPosition = new QPushButton(tr("Show position"), this /* parent */);
    connect(m_showPosition, &QPushButton::clicked, [this]() { emit OnShowPositionClicked(); });
    layout->addWidget(m_showPosition);
  }

  {
    auto * layout =
        BuildSubLayout<QVBoxLayout>(*mainLayout, *this /* parent */, &m_relatedQueriesBox);
    SetVerticalStretch(*m_relatedQueriesBox, 1 /* stretch */);

    layout->addWidget(new QLabel(tr("Related queries")));

    m_relatedQueries = new QListWidget();
    layout->addWidget(m_relatedQueries);
  }

  {
    auto * layout = BuildSubLayout<QHBoxLayout>(*mainLayout, *this /* parent */);

    m_markAllAsRelevant = new QPushButton(tr("Mark all as Relevant"), this /* parent */);
    connect(m_markAllAsRelevant, &QPushButton::clicked,
            [this]() { emit OnMarkAllAsRelevantClicked(); });
    layout->addWidget(m_markAllAsRelevant);

    m_markAllAsIrrelevant = new QPushButton(tr("Mark all as Irrelevant"), this /* parent */);
    connect(m_markAllAsIrrelevant, &QPushButton::clicked,
            [this]() { emit OnMarkAllAsIrrelevantClicked(); });
    layout->addWidget(m_markAllAsIrrelevant);
  }

  {
    m_uselessnessLabel = new QLabel(this /* parent */);
    m_uselessnessLabel->setText(tr("Sample is marked as useless"));
    m_uselessnessLabel->hide();
    mainLayout->addWidget(m_uselessnessLabel);
  }

  {
    auto * layout =
        BuildSubLayout<QVBoxLayout>(*mainLayout, *this /* parent */, &m_foundResultsBox);
    SetVerticalStretch(*m_foundResultsBox, 4 /* stretch */);

    {
      auto * subLayout = BuildSubLayout<QHBoxLayout>(*layout, *this /* parent */);
      subLayout->addWidget(new QLabel(tr("Found results")));

      m_spinner = new Spinner();
      subLayout->addWidget(&m_spinner->AsWidget());
    }

    m_foundResults = new ResultsView(*this /* parent */);
    layout->addWidget(m_foundResults);
  }

  {
    auto * layout =
        BuildSubLayout<QVBoxLayout>(*mainLayout, *this /* parent */, &m_nonFoundResultsBox);
    SetVerticalStretch(*m_nonFoundResultsBox, 2 /* stretch */);

    layout->addWidget(new QLabel(tr("Non found results")));

    m_nonFoundResults = new ResultsView(*this /* parent */);
    m_nonFoundResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_nonFoundResults, &ResultsView::customContextMenuRequested, [&](QPoint pos) {
      pos = m_nonFoundResults->mapToGlobal(pos);

      auto const items = m_nonFoundResults->selectedItems();
      for (auto const * item : items)
      {
        int const row = m_nonFoundResults->row(item);

        QMenu menu;
        auto const * action = menu.addAction("Remove result");
        connect(action, &QAction::triggered, [this, row]() { OnRemoveNonFoundResult(row); });

        menu.exec(pos);
      }
    });
    layout->addWidget(m_nonFoundResults);
  }

  setLayout(mainLayout);

  Clear();
}

void SampleView::SetContents(search::Sample const & sample,
                             std::optional<m2::PointD> const & position)
{
  if (!sample.m_query.empty())
  {
    m_query->setText(ToQString(sample.m_query));
    m_query->show();
  }
  if (!sample.m_locale.empty())
  {
    m_langs->setText(ToQString(sample.m_locale));
    m_langs->show();
  }
  m_showViewport->setEnabled(true);

  m_relatedQueries->clear();
  for (auto const & query : sample.m_relatedQueries)
    m_relatedQueries->addItem(ToQString(query));
  if (m_relatedQueries->count() != 0)
    m_relatedQueriesBox->show();

  ClearAllResults();
  m_position = position;
  if (m_position)
    ShowUserPosition(*m_position);
  else
    HideUserPosition();
}

void SampleView::OnSearchStarted()
{
  m_spinner->Show();
  m_showPosition->setEnabled(false);

  m_markAllAsRelevant->setEnabled(false);
  m_markAllAsIrrelevant->setEnabled(false);
}

void SampleView::OnSearchCompleted()
{
  m_spinner->Hide();
  auto const resultsAvailable = m_foundResults->HasResultsWithPoints();
  if (m_position)
  {
    if (resultsAvailable)
      m_showPosition->setText(tr("Show position and top results"));
    else
      m_showPosition->setText(tr("Show position"));
    m_showPosition->setEnabled(true);
  }
  else if (resultsAvailable)
  {
    m_showPosition->setText(tr("Show results"));
    m_showPosition->setEnabled(true);
  }
  else
  {
    m_showPosition->setEnabled(false);
  }

  m_markAllAsRelevant->setEnabled(resultsAvailable);
  m_markAllAsIrrelevant->setEnabled(resultsAvailable);
}

void SampleView::AddFoundResults(search::Results const & results)
{
  /// @todo Should clear previous m_foundResults.
  for (auto const & res : results)
    m_foundResults->Add(res);
}

void SampleView::ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                                     std::vector<ResultsEdits::Entry> const & entries)
{
  CHECK_EQUAL(results.size(), entries.size(), ());

  m_nonFoundResults->Clear();

  bool allDeleted = true;
  for (size_t i = 0; i < results.size(); ++i)
  {
    m_nonFoundResults->Add(results[i], entries[i]);
    if (!entries[i].m_deleted)
      allDeleted = false;
  }
  if (!allDeleted)
    m_nonFoundResultsBox->show();
}

void SampleView::ShowFoundResultsMarks(search::Results const & results)
{
  /// @todo Should clear previous _found_ results marks, but keep _nonfound_ if any.
  m_framework.FillSearchResultsMarks(false /* clear */, results);
}

void SampleView::ShowNonFoundResultsMarks(std::vector<search::Sample::Result> const & results,
                                          std::vector<ResultsEdits::Entry> const & entries)

{
  CHECK_EQUAL(results.size(), entries.size(), ());

  auto editSession = m_framework.GetBookmarkManager().GetEditSession();
  editSession.SetIsVisible(UserMark::Type::SEARCH, true);

  for (size_t i = 0; i < results.size(); ++i)
  {
    auto const & result = results[i];
    auto const & entry = entries[i];
    if (entry.m_deleted)
      continue;

    auto * mark = editSession.CreateUserMark<SearchMarkPoint>(result.m_pos);
    mark->SetNotFoundType();
  }
}

void SampleView::ClearSearchResultMarks()
{
  m_framework.GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::SEARCH);
}

void SampleView::ClearAllResults()
{
  m_foundResults->Clear();
  m_nonFoundResults->Clear();
  m_nonFoundResultsBox->hide();
  ClearSearchResultMarks();
}

void SampleView::SetResultsEdits(ResultsEdits & resultsResultsEdits,
                                 ResultsEdits & nonFoundResultsEdits)
{
  SetResultsEdits(*m_foundResults, resultsResultsEdits);
  SetResultsEdits(*m_nonFoundResults, nonFoundResultsEdits);
  m_nonFoundResultsEdits = &nonFoundResultsEdits;
}

void SampleView::OnUselessnessChanged(bool isUseless)
{
  if (isUseless)
  {
    m_uselessnessLabel->show();
    m_foundResultsBox->hide();
    m_nonFoundResultsBox->hide();
    m_markAllAsRelevant->hide();
    m_markAllAsIrrelevant->hide();
  }
  else
  {
    m_uselessnessLabel->hide();
    m_foundResultsBox->show();
    m_markAllAsRelevant->show();
    m_markAllAsIrrelevant->show();
  }
}

void SampleView::Clear()
{
  m_query->hide();
  m_langs->hide();

  m_showViewport->setEnabled(false);
  m_showPosition->setEnabled(false);

  m_markAllAsRelevant->setEnabled(false);
  m_markAllAsIrrelevant->setEnabled(false);

  m_relatedQueriesBox->hide();

  ClearAllResults();
  HideUserPosition();
  m_position = std::nullopt;
  OnSearchCompleted();
}

void SampleView::OnLocationChanged(Qt::DockWidgetArea area)
{
  if (area == Qt::RightDockWidgetArea)
    layout()->setContentsMargins(m_rightAreaMargins);
  else
    layout()->setContentsMargins(m_defaultMargins);
}

void SampleView::SetResultsEdits(ResultsView & results, ResultsEdits & edits)
{
  size_t const numRelevances = edits.GetRelevances().size();
  CHECK_EQUAL(results.Size(), numRelevances, ());
  for (size_t i = 0; i < numRelevances; ++i)
    results.Get(i).SetEditor(ResultsEdits::Editor(edits, i));
}

void SampleView::OnRemoveNonFoundResult(int row) { m_nonFoundResultsEdits->Delete(row); }

void SampleView::ShowUserPosition(m2::PointD const & position)
{
  // Clear the old position.
  HideUserPosition();

  auto es = m_framework.GetBookmarkManager().GetEditSession();
  auto mark = es.CreateUserMark<ColoredMarkPoint>(position);
  mark->SetColor(dp::Color(200, 100, 240, 255) /* purple */);
  mark->SetRadius(8.0f);
  m_positionMarkId = mark->GetId();
}

void SampleView::HideUserPosition()
{
  if (m_positionMarkId == kml::kInvalidMarkId)
    return;

  m_framework.GetBookmarkManager().GetEditSession().DeleteUserMark(m_positionMarkId);
  m_positionMarkId = kml::kInvalidMarkId;
}
