#pragma once

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include "geometry/point2d.hpp"

#include <QtCore/QMargins>
#include <QtWidgets/QWidget>

class Framework;
class QLabel;
class QListWidget;
class QPushButton;
class ResultsView;
class Spinner;

class SampleView : public QWidget
{
  Q_OBJECT

public:
  using Relevance = search::Sample::Result::Relevance;

  SampleView(QWidget * parent, Framework & framework);

  void SetContents(search::Sample const & sample, bool positionAvailable,
                   m2::PointD const & position);
  void OnSearchStarted();
  void OnSearchCompleted();

  void AddFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end);
  void ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                           std::vector<Edits::Entry> const & entries);

  void ShowFoundResultsMarks(search::Results::ConstIter begin, search::Results::ConstIter end);
  void ShowNonFoundResultsMarks(std::vector<search::Sample::Result> const & results,
                                std::vector<Edits::Entry> const & entries);
  void ClearSearchResultMarks();

  void SetEdits(Edits & resultsEdits, Edits & nonFoundResultsEdits);

  void Clear();

  ResultsView & GetFoundResultsView() { return *m_foundResults; }
  ResultsView & GetNonFoundResultsView() { return *m_nonFoundResults; }

  void OnLocationChanged(Qt::DockWidgetArea area);

signals:
  void OnShowViewportClicked();
  void OnShowPositionClicked();
  void OnMarkAllAsRelevantClicked();
  void OnMarkAllAsIrrelevantClicked();

private:
  void ClearAllResults();
  void SetEdits(ResultsView & results, Edits & edits);
  void OnRemoveNonFoundResult(int row);

  void ShowUserPosition(m2::PointD const & position);
  void HideUserPosition();

  Framework & m_framework;

  Spinner * m_spinner = nullptr;

  QLabel * m_query = nullptr;
  QLabel * m_langs = nullptr;

  QListWidget * m_relatedQueries = nullptr;
  QWidget * m_relatedQueriesBox = nullptr;

  QPushButton * m_showViewport = nullptr;
  QPushButton * m_showPosition = nullptr;

  QPushButton * m_markAllAsRelevant = nullptr;
  QPushButton * m_markAllAsIrrelevant = nullptr;

  ResultsView * m_foundResults = nullptr;
  QWidget * m_foundResultsBox = nullptr;

  ResultsView * m_nonFoundResults = nullptr;
  QWidget * m_nonFoundResultsBox = nullptr;

  Edits * m_nonFoundResultsEdits = nullptr;

  QMargins m_rightAreaMargins;
  QMargins m_defaultMargins;

  bool m_positionAvailable = false;
};
