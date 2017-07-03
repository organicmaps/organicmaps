#pragma once

#include "openlr/openlr_match_quality/assessment_tool/traffic_mode.hpp"

#include "base/string_utils.hpp"

#include <QMainWindow>

class Framework;
class TrafficMode;

namespace df
{
class DrapeApi;
}  // namespace df

class QDockWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(Framework & framework);

private:
  void CreateTrafficPanel(string const & dataFilePath, string const & sampleFilePath);
  void DestroyTrafficPanel();

  void OnOpenTrafficSample();
  void OnCloseTrafficSample();
  void OnSaveTrafficSample();

  Framework & m_framework;

  TrafficMode * m_trafficMode = nullptr;
  QDockWidget * m_docWidget = nullptr;

  QAction * m_saveTrafficSampleAction = nullptr;
  QAction * m_closeTrafficSampleAction = nullptr;
};
