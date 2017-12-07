#pragma once

#include "search/search_quality/assessment_tool/view.hpp"

#include "indexer/feature_decl.hpp"

#include <QtWidgets/QMainWindow>

class Framework;
class QDockWidget;
class QItemSelection;
class SamplesView;
class SampleView;

namespace qt
{
namespace common
{
class MapWidget;
}
}

class MainView : public QMainWindow, public View
{
  Q_OBJECT

public:
  explicit MainView(Framework & framework);
  ~MainView() override;

  // View overrides:
  void SetSamples(ContextList::SamplesSlice const & samples) override;
  void OnSearchStarted() override;
  void OnSearchCompleted() override;
  void ShowSample(size_t sampleIndex, search::Sample const & sample, bool positionAvailable,
                  m2::PointD const & position, bool hasEdits) override;

  void AddFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end) override;
  void ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                           std::vector<Edits::Entry> const & entries) override;

  void ShowFoundResultsMarks(search::Results::ConstIter begin,
                             search::Results::ConstIter end) override;
  void ShowNonFoundResultsMarks(std::vector<search::Sample::Result> const & results,
                                std::vector<Edits::Entry> const & entries) override;
  void ClearSearchResultMarks() override;

  void MoveViewportToResult(search::Result const & result) override;
  void MoveViewportToResult(search::Sample::Result const & result) override;
  void MoveViewportToRect(m2::RectD const & rect) override;

  void OnResultChanged(size_t sampleIndex, ResultType type, Edits::Update const & update) override;
  void SetEdits(size_t sampleIndex, Edits & foundResultsEdits,
                Edits & nonFoundResultsEdits) override;
  void OnSampleChanged(size_t sampleIndex, bool hasEdits) override;
  void OnSamplesChanged(bool hasEdits) override;

  void ShowError(std::string const & msg) override;

  void Clear() override;

protected:
  // QMainWindow overrides:
  void closeEvent(QCloseEvent * event) override;

private slots:
  void OnSampleSelected(QItemSelection const & current);
  void OnResultSelected(QItemSelection const & current);
  void OnNonFoundResultSelected(QItemSelection const & current);

private:
  enum class State
  {
    BeforeSearch,
    Search,
    AfterSearch
  };

  friend string DebugPrint(State state)
  {
    switch (state)
    {
    case State::BeforeSearch: return "BeforeSearch";
    case State::Search: return "Search";
    case State::AfterSearch: return "AfterSearch";
    }
  }

  enum class SaveResult
  {
    NoEdits,
    Saved,
    Discarded,
    Cancelled
  };

  void InitMapWidget();
  void InitDocks();
  void InitMenuBar();

  void Open();
  void Save();
  void SaveAs();

  void SetSamplesDockTitle(bool hasEdits);
  void SetSampleDockTitle(bool hasEdits);
  SaveResult TryToSaveEdits(QString const & msg);

  void AddSelectedFeature(QPoint const & p);

  QDockWidget * CreateDock(QWidget & widget);

  Framework & m_framework;

  SamplesView * m_samplesView = nullptr;
  QDockWidget * m_samplesDock = nullptr;

  SampleView * m_sampleView = nullptr;
  QDockWidget * m_sampleDock = nullptr;

  QAction * m_save = nullptr;
  QAction * m_saveAs = nullptr;

  State m_state = State::BeforeSearch;
  FeatureID m_selectedFeature;

  bool m_skipFeatureInfoDialog = false;
  std::string m_sampleLocale;
};
