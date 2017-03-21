#pragma once

#include <cstddef>
#include <vector>

#include <QtWidgets/QListWidget>

class QWidget;
class ResultView;

namespace search
{
class Result;
}

class ResultsView : private QListWidget
{
public:
  explicit ResultsView(QWidget & parent);

  void Add(search::Result const & result);

  ResultView & Get(size_t i);
  ResultView const & Get(size_t i) const;

  size_t Size() const { return m_results.size(); }

  void Clear();

  QWidget * GetWidget() { return this; }

private:
  std::vector<ResultView *> m_results;
};
