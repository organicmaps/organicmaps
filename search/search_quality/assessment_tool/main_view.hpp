#pragma once

#include "search/search_quality/assessment_tool/view.hpp"

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTableView>

#include <memory>

class QItemSelection;

namespace qt
{
namespace common
{
class MapWidget;
}
}

class Framework;

class MainView : public QMainWindow, public View
{
  Q_OBJECT

public:
  explicit MainView(Framework & framework);

  // View overrides:
  void SetSamples(std::vector<search::Sample> const & samples) override;
  void ShowSample(search::Sample const & sample) override;
  void ShowError(std::string const & msg) override;

private Q_SLOTS:
  void OnSampleSelected(QItemSelection const & current);

private:
  void InitMenuBar();
  void InitMapWidget();
  void InitDocks();

  void Open();

  Framework & m_framework;

  std::unique_ptr<QStandardItemModel> m_samplesModel;
  std::unique_ptr<QTableView> m_samplesTable;
  std::unique_ptr<QDockWidget> m_samplesDock;
};
