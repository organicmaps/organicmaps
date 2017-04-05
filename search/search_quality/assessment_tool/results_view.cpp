#include "search/search_quality/assessment_tool/results_view.hpp"

#include "search/search_quality/assessment_tool/result_view.hpp"

#include "base/assert.hpp"

#include <QtWidgets/QListWidgetItem>

ResultsView::ResultsView(QWidget & parent) : QListWidget(&parent) { setAlternatingRowColors(true); }

void ResultsView::Add(search::Result const & result)
{
  auto * item = new QListWidgetItem(this /* parent */);
  addItem(item);

  auto * view = new ResultView(result, *this /* parent */);
  item->setSizeHint(view->minimumSizeHint());
  setItemWidget(item, view);

  m_results.push_back(view);
}

ResultView & ResultsView::Get(size_t i)
{
  CHECK_LESS(i, Size(), ());
  return *m_results[i];
}

ResultView const & ResultsView::Get(size_t i) const
{
  CHECK_LESS(i, Size(), ());
  return *m_results[i];
}

void ResultsView::Update(Edits::Update const & update)
{
  switch (update.m_type)
  {
  case Edits::Update::Type::SingleRelevance:
    CHECK_LESS(update.m_index, m_results.size(), ());
    m_results[update.m_index]->Update();
    break;
  case Edits::Update::Type::AllRelevances:
    for (auto * result : m_results)
      result->Update();
    break;
  }
}

void ResultsView::Clear()
{
  m_results.clear();
  clear();
}
