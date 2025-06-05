#pragma once

#include "openlr/openlr_match_quality/openlr_assessment_tool/traffic_mode.hpp"

#include "base/string_utils.hpp"

#include <string>

#include <QMainWindow>

class Framework;
class QHBoxLayout;

namespace openlr
{
class MapWidget;
class TrafficMode;
class WebView;
}  // namespace openlr

namespace df
{
class DrapeApi;
}

class QDockWidget;

namespace openlr
{
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(Framework & framework);

private:
  void CreateTrafficPanel(std::string const & dataFilePath);
  void DestroyTrafficPanel();

  void OnOpenTrafficSample();
  void OnCloseTrafficSample();
  void OnSaveTrafficSample();
  void OnPathEditingStop();

  Framework & m_framework;

  openlr::TrafficMode * m_trafficMode = nullptr;
  QDockWidget * m_docWidget = nullptr;

  QAction * m_goldifyMatchedPathAction = nullptr;
  QAction * m_saveTrafficSampleAction = nullptr;
  QAction * m_closeTrafficSampleAction = nullptr;
  QAction * m_startEditingAction = nullptr;
  QAction * m_commitPathAction = nullptr;
  QAction * m_cancelPathAction = nullptr;
  QAction * m_ignorePathAction = nullptr;

  openlr::MapWidget * m_mapWidget = nullptr;
  QHBoxLayout * m_layout = nullptr;
};
}  // namespace openlr
