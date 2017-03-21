#pragma once

#include "search/search_quality/sample.hpp"

#include <QtWidgets/QWidget>

class QLabel;
class QRadioButton;

namespace search
{
class Result;
}

class ResultView : public QWidget
{
public:
  ResultView(search::Result const & result, QWidget & parent);

  void SetRelevance(search::Sample::Result::Relevance relevance);

private:
  void Init();
  void SetContents(search::Result const & result);

  QLabel * m_string = nullptr;
  QLabel * m_type = nullptr;
  QLabel * m_address = nullptr;

  QRadioButton * m_irrelevant = nullptr;
  QRadioButton * m_relevant = nullptr;
  QRadioButton * m_vital = nullptr;
};
