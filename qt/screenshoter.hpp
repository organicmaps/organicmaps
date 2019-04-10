#pragma once

#include "drape_frontend/visual_params.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/rect2d.hpp"

#include <QtWidgets/QWidget>

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
  Screenshoter(ScreenshotParams const & screenshotParams, Framework & framework, QWidget * widget);

  void Start();

  void OnCountryChanged(storage::CountryId countryId);
  void OnViewportChanged();
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
    Done
  };

  void ProcessNextKml();
  void PrepareCountries();
  void SaveScreenshot();
  void WaitGraphics();
  void ChangeState(State newState);

  friend std::string DebugPrint(State state);

  State m_state = State::NotStarted;
  ScreenshotParams m_screenshotParams;
  Framework & m_framework;
  QWidget * m_widget;
  std::list<std::string> m_filesToProcess;
  size_t m_filesCount = 0;
  std::set<storage::CountryId> m_countriesToDownload;
  std::string m_nextScreenshotName;
};

std::string DebugPrint(Screenshoter::State state);
}  // namespace qt
