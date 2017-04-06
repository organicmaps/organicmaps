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
#include <QtWidgets/QVBoxLayout>

SampleView::SampleView(QWidget * parent) : QWidget(parent)
{
  auto * mainLayout = BuildLayoutWithoutMargins<QVBoxLayout>(this /* parent */);

  {
    auto * box = new QWidget(this /* parent */);
    auto * layout = BuildLayoutWithoutMargins<QHBoxLayout>(box /* parent */);
    box->setLayout(layout);

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

    mainLayout->addWidget(box);
  }

  {
    auto * box = new QWidget(this /* parent */);
    auto * layout = BuildLayoutWithoutMargins<QVBoxLayout>(box /* parent */);
    box->setLayout(layout);

    layout->addWidget(new QLabel(tr("Found results")));

    m_results = new ResultsView(*box /* parent */);
    layout->addWidget(m_results);

    mainLayout->addWidget(box);
  }

  setLayout(mainLayout);

  Clear();
}

void SampleView::SetContents(search::Sample const & sample)
{
  m_query->setText(ToQString(sample.m_query));
  m_query->home(false /* mark */);

  m_langs->Select(sample.m_locale);

  m_results->Clear();
}

void SampleView::ShowResults(search::Results::Iter begin, search::Results::Iter end)
{
  for (auto it = begin; it != end; ++it)
    m_results->Add(*it /* result */);
}

void SampleView::EnableEditing(Edits & edits)
{
  m_edits = &edits;

  size_t const numRelevances = m_edits->GetRelevances().size();
  CHECK_EQUAL(m_results->Size(), numRelevances, ());
  for (size_t i = 0; i < numRelevances; ++i)
    m_results->Get(i).EnableEditing(Edits::RelevanceEditor(*m_edits, i));
}

void SampleView::Update(Edits::Update const & update) { m_results->Update(update); }

void SampleView::Clear()
{
  m_query->setText(QString());
  m_langs->Select("default");
  m_results->Clear();
  m_edits = nullptr;
}
