#pragma once

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include <QtWidgets/QWidget>

class LanguagesList;
class QLineEdit;
class ResultsView;

class SampleView : public QWidget
{
public:
  explicit SampleView(QWidget * parent);

  void SetContents(search::Sample const & sample);
  void ShowResults(search::Results::Iter begin, search::Results::Iter end);
  void SetResultRelevances(std::vector<search::Sample::Result::Relevance> const & relevances);

private:
  QLineEdit * m_query = nullptr;
  LanguagesList * m_langs = nullptr;
  ResultsView * m_results = nullptr;
};
