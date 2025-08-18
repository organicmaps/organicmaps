#include "search/search_quality/assessment_tool/results_view.hpp"

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/result_view.hpp"

#include "base/assert.hpp"

#include <QtWidgets/QListWidgetItem>

ResultsView::ResultsView(QWidget & parent) : QListWidget(&parent)
{
  setAlternatingRowColors(true);

  connect(selectionModel(), &QItemSelectionModel::selectionChanged, [&](QItemSelection const & current)
  {
    auto const indexes = current.indexes();
    for (auto const & index : indexes)
      emit OnResultSelected(index.row());
  });

  connect(this, &ResultsView::itemClicked, [&](QListWidgetItem * item)
  {
    auto const index = indexFromItem(item);
    emit OnResultSelected(index.row());
  });
}

void ResultsView::Add(search::Result const & result)
{
  AddImpl(result, false /* hidden */);
  if (result.HasPoint())
    m_hasResultsWithPoints = true;
}

void ResultsView::Add(search::Sample::Result const & result, ResultsEdits::Entry const & entry)
{
  AddImpl(result, entry.m_deleted /* hidden */);
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

void ResultsView::Update(ResultsEdits::Update const & update)
{
  switch (update.m_type)
  {
  case ResultsEdits::Update::Type::Single:
  {
    CHECK_LESS(update.m_index, m_results.size(), ());
    m_results[update.m_index]->Update();
    break;
  }
  case ResultsEdits::Update::Type::All:
  {
    for (auto * result : m_results)
      result->Update();
    break;
  }
  case ResultsEdits::Update::Type::Add:
  {
    CHECK_LESS(update.m_index, m_results.size(), ());
    m_results[update.m_index]->Update();
    break;
  }
  case ResultsEdits::Update::Type::Delete:
  {
    auto const index = update.m_index;
    CHECK_LESS(index, Size(), ());
    item(static_cast<int>(index))->setHidden(true);
    break;
  }
  case ResultsEdits::Update::Type::Resurrect:
    auto const index = update.m_index;
    CHECK_LESS(index, Size(), ());
    item(static_cast<int>(index))->setHidden(false);
    break;
  };
}

void ResultsView::Clear()
{
  m_results.clear();
  m_hasResultsWithPoints = false;
  clear();
}

template <typename Result>
void ResultsView::AddImpl(Result const & result, bool hidden)
{
  auto * item = new QListWidgetItem(this /* parent */);
  item->setHidden(hidden);
  addItem(item);

  auto * view = new ResultView(result, *this /* parent */);
  item->setSizeHint(view->minimumSizeHint());
  setItemWidget(item, view);

  m_results.push_back(view);
}
