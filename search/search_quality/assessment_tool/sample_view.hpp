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

  void SetContents(search::Sample const & sample);
  void ShowResults(search::Results::Iter begin, search::Results::Iter end);

  void EnableEditing(Edits & edits);

  void Update(Edits::Update const & update);
  void Clear();

  ResultsView & GetResultsView() { return *m_results; }

signals:
  void OnShowViewportClicked();
  void OnShowPositionClicked();

private:
  QLineEdit * m_query = nullptr;
  LanguagesList * m_langs = nullptr;
  QPushButton * m_showViewport = nullptr;
  QPushButton * m_showPosition = nullptr;
  ResultsView * m_results = nullptr;

  Edits * m_edits = nullptr;
};
