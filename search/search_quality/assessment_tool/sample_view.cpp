#include "search/search_quality/assessment_tool/sample_view.hpp"

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/helpers.hpp"
#include "search/search_quality/assessment_tool/languages_list.hpp"
#include "search/search_quality/assessment_tool/result_view.hpp"
#include "search/search_quality/assessment_tool/results_view.hpp"
#include "search/search_quality/sample.hpp"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace
{
template <typename Layout>
Layout * BuildSubLayout(QLayout & mainLayout, QWidget & parent)
{
  auto * box = new QWidget(&parent);
  auto * subLayout = BuildLayoutWithoutMargins<Layout>(box /* parent */);
  box->setLayout(subLayout);
  mainLayout.addWidget(box);
  return subLayout;
}
}  // namespace

SampleView::SampleView(QWidget * parent) : QWidget(parent)
{
  auto * mainLayout = BuildLayoutWithoutMargins<QVBoxLayout>(this /* parent */);

  {
    auto * layout = BuildSubLayout<QHBoxLayout>(*mainLayout, *this /* parent */);

    m_query = new QLineEdit(this /* parent */);
    m_query->setToolTip(tr("Query text"));

    // TODO (@y): enable this as soon as editing of query will be
    // ready.
    m_query->setEnabled(false);
    layout->addWidget(m_query);

    m_langs = new LanguagesList(this /* parent */);
    m_langs->setToolTip(tr("Query input language"));

    // TODO (@y): enable this as soon as editing of input language
    // will be ready.
    m_langs->setEnabled(false);
    layout->addWidget(m_langs);
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
    auto * layout = BuildSubLayout<QVBoxLayout>(*mainLayout, *this /* parent */);

    layout->addWidget(new QLabel(tr("Found results")));

    m_foundResults = new ResultsView(*this /* parent */);
    layout->addWidget(m_foundResults);
  }

  {
    auto * layout = BuildSubLayout<QVBoxLayout>(*mainLayout, *this /* parent */);

    layout->addWidget(new QLabel(tr("Non found results")));

    m_nonFoundResults = new ResultsView(*this /* parent */);
    layout->addWidget(m_nonFoundResults);
  }
  setLayout(mainLayout);

  Clear();
}

void SampleView::SetContents(search::Sample const & sample, bool positionAvailable)
{
  m_query->setText(ToQString(sample.m_query));
  m_query->home(false /* mark */);

  m_langs->Select(sample.m_locale);
  m_showViewport->setEnabled(true);
  m_showPosition->setEnabled(positionAvailable);

  m_foundResults->Clear();
  m_nonFoundResults->Clear();
}

void SampleView::ShowFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end)
{
  for (auto it = begin; it != end; ++it)
    m_foundResults->Add(*it /* result */);
}

void SampleView::ShowNonFoundResults(std::vector<search::Sample::Result> const & results)
{
  for (auto const & result : results)
    m_nonFoundResults->Add(result);
}

void SampleView::EnableEditing(Edits & resultsEdits, Edits & nonFoundResultsEdits)
{
  EnableEditing(*m_foundResults, resultsEdits);
  EnableEditing(*m_nonFoundResults, nonFoundResultsEdits);
}

void SampleView::Clear()
{
  m_query->setText(QString());
  m_langs->Select("default");
  m_showViewport->setEnabled(false);
  m_showPosition->setEnabled(false);
  m_foundResults->Clear();
  m_nonFoundResults->Clear();
}

void SampleView::EnableEditing(ResultsView & results, Edits & edits)
{
  size_t const numRelevances = edits.GetRelevances().size();
  CHECK_EQUAL(results.Size(), numRelevances, ());
  for (size_t i = 0; i < numRelevances; ++i)
    results.Get(i).EnableEditing(Edits::RelevanceEditor(edits, i));
}
