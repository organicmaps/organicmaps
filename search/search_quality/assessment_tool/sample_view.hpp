#pragma once

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include <QtWidgets/QWidget>

class LanguagesList;
class QLineEdit;
class QPushButton;
class ResultsView;

class SampleView : public QWidget
{
  Q_OBJECT

public:
  using Relevance = search::Sample::Result::Relevance;

  explicit SampleView(QWidget * parent);

  void SetContents(search::Sample const & sample, bool positionAvailable);
  void ShowFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end);
  void ShowNonFoundResults(std::vector<search::Sample::Result> const & results);

  void EnableEditing(Edits & resultsEdits, Edits & nonFoundResultsEdits);

  void Clear();

  ResultsView & GetFoundResultsView() { return *m_foundResults; }
  ResultsView & GetNonFoundResultsView() { return *m_nonFoundResults; }

signals:
  void OnShowViewportClicked();
  void OnShowPositionClicked();

private:
  void EnableEditing(ResultsView & results, Edits & edits);

  QLineEdit * m_query = nullptr;
  LanguagesList * m_langs = nullptr;
  QPushButton * m_showViewport = nullptr;
  QPushButton * m_showPosition = nullptr;
  ResultsView * m_foundResults = nullptr;
  ResultsView * m_nonFoundResults = nullptr;
};
