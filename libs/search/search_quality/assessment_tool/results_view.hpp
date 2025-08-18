#pragma once

#include "search/search_quality/assessment_tool/edits.hpp"

#include <cstddef>
#include <vector>

#include <QtWidgets/QListWidget>

class QWidget;
class ResultView;

namespace search
{
class Result;
}

class ResultsView : public QListWidget
{
  Q_OBJECT

public:
  explicit ResultsView(QWidget & parent);

  void Add(search::Result const & result);
  void Add(search::Sample::Result const & result, ResultsEdits::Entry const & entry);

  ResultView & Get(size_t i);
  ResultView const & Get(size_t i) const;
  void Update(ResultsEdits::Update const & update);

  size_t Size() const { return m_results.size(); }
  bool HasResultsWithPoints() const { return m_hasResultsWithPoints; }

  void Clear();

signals:
  void OnResultSelected(int index);

private:
  template <typename Result>
  void AddImpl(Result const & result, bool hidden);

  std::vector<ResultView *> m_results;
  bool m_hasResultsWithPoints = false;
};
