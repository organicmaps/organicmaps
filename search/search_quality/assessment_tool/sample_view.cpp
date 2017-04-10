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

    m_results = new ResultsView(*this /* parent */);
    layout->addWidget(m_results);
  }

  setLayout(mainLayout);

  Clear();
}

void SampleView::SetContents(search::Sample const & sample)
{
  m_query->setText(ToQString(sample.m_query));
  m_query->home(false /* mark */);

  m_langs->Select(sample.m_locale);
  m_showViewport->setEnabled(true);
  m_showPosition->setEnabled(true);

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
  m_showViewport->setEnabled(false);
  m_showPosition->setEnabled(false);
  m_results->Clear();
  m_edits = nullptr;
}
