#pragma once

#include "storage/storage_defines.hpp"

#include "drape_frontend/visual_params.hpp"

#include "geometry/rect2d.hpp"

#include <QOpenGLWidget>

#include <list>
#include <string>

class Framework;

namespace qt
{
struct ScreenshotParams
{
  static uint32_t constexpr kDefaultWidth = 576;
  static uint32_t constexpr kDefaultHeight = 720;

  using TStatusChangedFn = std::function<void(std::string const & /* status */, bool finished)>;

  enum class Mode
  {
    Points,
    Rects,
    KmlFiles
  };

  Mode m_mode = Mode::Points;
  std::string m_points;
  std::string m_rects;
  std::string m_kmlPath;
  std::string m_dstPath;
  uint32_t m_width = kDefaultWidth;
  uint32_t m_height = kDefaultHeight;
  double m_dpiScale = df::VisualParams::kXhdpiScale;
  TStatusChangedFn m_statusChangedFn;
};

class Screenshoter
{
public:
  Screenshoter(ScreenshotParams const & screenshotParams, Framework & framework, QOpenGLWidget * widget);

  void Start();

  void OnCountryChanged(storage::CountryId countryId);

  void OnPositionReady();
  void OnGraphicsReady();

private:
  enum class State : uint8_t
  {
    NotStarted,
    LoadKml,
    WaitPosition,
    WaitCountries,
    WaitGraphics,
    Ready,
    FileError,
    ParamsError,
    Done
  };

  void ProcessNextItem();
  void PrepareToProcessKml();
  void ProcessNextKml();
  void ProcessNextRect();
  void ProcessNextPoint();
  void PrepareCountries();
  void SaveScreenshot();
  void WaitPosition();
  void WaitGraphics();
  void ChangeState(State newState, std::string const & msg = "");

  size_t GetItemsToProcessCount();

  bool PrepareItemsToProcess();
  bool PrepareKmlFiles();
  bool PreparePoints();
  bool PrepareRects();

  bool LoadRects(std::string const & rects);
  bool LoadPoints(std::string const & points);

  friend std::string DebugPrint(State state);

  State m_state = State::NotStarted;
  ScreenshotParams m_screenshotParams;
  Framework & m_framework;
  QOpenGLWidget * m_widget;
  std::list<std::string> m_filesToProcess;
  std::list<std::pair<m2::PointD, int>> m_pointsToProcess;
  std::list<m2::RectD> m_rectsToProcess;
  size_t m_itemsCount = 0;
  std::set<storage::CountryId> m_countriesToDownload;
  std::string m_nextScreenshotName;
};

std::string DebugPrint(Screenshoter::State state);
std::string DebugPrint(ScreenshotParams::Mode mode);
}  // namespace qt
