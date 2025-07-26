#pragma once
#include "edits.hpp"

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include "geometry/point2d.hpp"

#include "kml/type_utils.hpp"

#include <optional>

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

  void SetContents(search::Sample const & sample, std::optional<m2::PointD> const & position);
  void OnSearchStarted();
  void OnSearchCompleted();

  void AddFoundResults(search::Results const & results);
  void ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                           std::vector<ResultsEdits::Entry> const & entries);

  void ShowFoundResultsMarks(search::Results const & results);
  void ShowNonFoundResultsMarks(std::vector<search::Sample::Result> const & results,
                                std::vector<ResultsEdits::Entry> const & entries);
  void ClearSearchResultMarks();

  void SetResultsEdits(ResultsEdits & resultsResultsEdits, ResultsEdits & nonFoundResultsResultsEdits);

  void OnUselessnessChanged(bool isUseless);

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
  void SetResultsEdits(ResultsView & results, ResultsEdits & edits);
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

  QLabel * m_uselessnessLabel = nullptr;

  ResultsView * m_foundResults = nullptr;
  QWidget * m_foundResultsBox = nullptr;

  ResultsView * m_nonFoundResults = nullptr;
  QWidget * m_nonFoundResultsBox = nullptr;

  ResultsEdits * m_nonFoundResultsEdits = nullptr;

  QMargins m_rightAreaMargins;
  QMargins m_defaultMargins;

  kml::MarkId m_positionMarkId = kml::kInvalidMarkId;

  std::optional<m2::PointD> m_position;
};
