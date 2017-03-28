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
  void ShowSample(size_t index, search::Sample const & sample, bool hasEdits) override;
  void ShowResults(search::Results::Iter begin, search::Results::Iter end) override;

  void OnSampleChanged(size_t index, bool hasEdits) override;
  void EnableSampleEditing(size_t index, Edits & edits) override;
  void OnSamplesChanged(bool hasEdits) override;

  void ShowError(std::string const & msg) override;

private Q_SLOTS:
  void OnSampleSelected(QItemSelection const & current);

private:
  void InitMapWidget();
  void InitDocks();
  void InitMenuBar();

  void Open();

  QDockWidget * CreateDock(std::string const & title, QWidget & widget);

  Framework & m_framework;

  SamplesView * m_samplesView = nullptr;
  QDockWidget * m_samplesDock = nullptr;

  SampleView * m_sampleView = nullptr;
  QDockWidget * m_sampleDock = nullptr;
};
