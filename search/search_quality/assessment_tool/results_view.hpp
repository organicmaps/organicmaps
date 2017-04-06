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
public:
  explicit ResultsView(QWidget & parent);

  void Add(search::Result const & result);

  ResultView & Get(size_t i);
  ResultView const & Get(size_t i) const;
  void Update(Edits::Update const & update);

  size_t Size() const { return m_results.size(); }

  void Clear();

private:
  std::vector<ResultView *> m_results;
};
