#pragma once

#include "search/search_quality/assessment_tool/view.hpp"

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
  void ShowSample(size_t sampleIndex, search::Sample const & sample, bool positionAvailable,
                  bool hasEdits) override;
  void ShowFoundResults(search::Results::ConstIter begin, search::Results::ConstIter end) override;
  void ShowNonFoundResults(std::vector<search::Sample::Result> const & results) override;

  void MoveViewportToResult(search::Result const & result) override;
  void MoveViewportToResult(search::Sample::Result const & result) override;
  void MoveViewportToRect(m2::RectD const & rect) override;

  void OnResultChanged(size_t sampleIndex, ResultType type, Edits::Update const & update) override;
  void EnableSampleEditing(size_t sampleIndex, Edits & foundResultsEdits,
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

  QDockWidget * CreateDock(QWidget & widget);

  Framework & m_framework;

  SamplesView * m_samplesView = nullptr;
  QDockWidget * m_samplesDock = nullptr;

  SampleView * m_sampleView = nullptr;
  QDockWidget * m_sampleDock = nullptr;

  QAction * m_save = nullptr;
  QAction * m_saveAs = nullptr;
};
