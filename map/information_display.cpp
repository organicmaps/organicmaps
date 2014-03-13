#include "information_display.hpp"
#include "drawer.hpp"
#include "country_status_display.hpp"
#include "compass_arrow.hpp"
#include "framework.hpp"

#include "../indexer/mercator.hpp"

#include "../gui/controller.hpp"
#include "../gui/button.hpp"
#include "../gui/cached_text_view.hpp"

#include "../graphics/defines.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/straight_text_element.hpp"
#include "../graphics/depth_constants.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../base/mutex.hpp"
#include "../base/macros.hpp"

#include "../geometry/transformations.hpp"

#include "../std/fstream.hpp"
#include "../std/iomanip.hpp"
#include "../std/target_os.hpp"

namespace
{
  static int const RULLER_X_OFFSET = 65;
  static int const RULLER_Y_OFFSET = 15;
  static int const FONT_SIZE = 14;
  static int const COMPASS_W_OFFSET = 13;
  static int const COMPASS_H_OFFSET = 71;
}

InformationDisplay::InformationDisplay(Framework * fw)
  : m_bottomShift(0),
    m_visualScale(1)
{
  m_fontDesc.m_color = graphics::Color(0x44, 0x44, 0x44, 0xFF);

  InitRuler(fw);
  InitCountryStatusDisplay(fw);
  InitCompassArrow(fw);
  InitLocationState(fw);
  InitDebugLabel();

  enableDebugPoints(false);
  enableDebugInfo(false);
  enableMemoryWarning(false);
  enableBenchmarkInfo(false);
  enableCountryStatusDisplay(false);
  m_compassArrow->setIsVisible(false);
  m_ruler->setIsVisible(false);

  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    m_DebugPts[i] = m2::PointD(0, 0);

  setVisualScale(m_visualScale);
}

void InformationDisplay::InitRuler(Framework * fw)
{
  Ruler::Params p;

  p.m_depth = graphics::rulerDepth;
  p.m_position = graphics::EPosAboveRight;
  p.m_framework = fw;

  m_ruler.reset(new Ruler(p));
}

void InformationDisplay::InitCountryStatusDisplay(Framework * fw)
{
  CountryStatusDisplay::Params p;

  p.m_pivot = m2::PointD(0, 0);
  p.m_position = graphics::EPosCenter;
  p.m_depth = graphics::countryStatusDepth;
  p.m_storage = &fw->Storage();

  m_countryStatusDisplay.reset(new CountryStatusDisplay(p));
}

void InformationDisplay::InitCompassArrow(Framework * fw)
{
  CompassArrow::Params p;

  p.m_position = graphics::EPosCenter;
  p.m_depth = graphics::compassDepth;
  p.m_pivot = m2::PointD(0, 0);
  p.m_framework = fw;

  m_compassArrow.reset(new CompassArrow(p));
}

void InformationDisplay::InitLocationState(Framework * fw)
{
  location::State::Params p;

  p.m_position = graphics::EPosCenter;
  p.m_depth = graphics::locationDepth;
  p.m_pivot = m2::PointD(0, 0);
  p.m_locationAreaColor = graphics::Color(0x51, 0xA3, 0xDC, 0x25);
  p.m_framework = fw;

  m_locationState.reset(new location::State(p));
}

void InformationDisplay::InitDebugLabel()
{
  gui::CachedTextView::Params p;

  p.m_depth = graphics::debugDepth;
  p.m_position = graphics::EPosAboveRight;
  p.m_pivot = m2::PointD(0, 0);

  m_debugLabel.reset(new gui::CachedTextView(p));
}

void InformationDisplay::setController(gui::Controller *controller)
{
  m_controller = controller;
  m_controller->AddElement(m_countryStatusDisplay);
  m_controller->AddElement(m_compassArrow);
  m_controller->AddElement(m_locationState);
  m_controller->AddElement(m_ruler);
  m_controller->AddElement(m_debugLabel);
}

void InformationDisplay::setScreen(ScreenBase const & screen)
{
  m_screen = screen;

  if (m_countryStatusDisplay->isVisible())
  {
    m2::RectD pxRect = m_screen.PixelRect();
    m2::PointD pt = m2::PointD(pxRect.SizeX() / 2, pxRect.SizeY() / 2) - m2::PointD(0, m_bottomShift * m_visualScale);
    m_countryStatusDisplay->setPivot(pt);
  }

  double k = m_controller->GetVisualScale();
  m2::PointD size = m_compassArrow->GetPixelSize();

  m_compassArrow->setPivot(m2::PointD(COMPASS_W_OFFSET * k + size.x / 2.0,
                                      COMPASS_H_OFFSET * k + size.y / 2.0));
}

void InformationDisplay::setBottomShift(double bottomShift)
{
  m_bottomShift = bottomShift;
}

void InformationDisplay::setDisplayRect(m2::RectI const & rect)
{
  m_displayRect = rect;

  m2::PointD pt(m2::PointD(m_displayRect.minX() + RULLER_X_OFFSET * m_visualScale,
                           m_displayRect.maxY() - RULLER_Y_OFFSET * m_visualScale));

  m_ruler->setPivot(pt);

  m2::PointD debugLabelPivot(m_displayRect.minX() + 10,
                             m_displayRect.minY() + 30 + 5 * m_visualScale);

  m_debugLabel->setPivot(debugLabelPivot);
}

void InformationDisplay::enableDebugPoints(bool doEnable)
{
  m_isDebugPointsEnabled = doEnable;
}

void InformationDisplay::setDebugPoint(int pos, m2::PointD const & pt)
{
  m_DebugPts[pos] = pt;
}

void InformationDisplay::drawDebugPoints(Drawer * pDrawer)
{
  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    if (m_DebugPts[i] != m2::PointD(0, 0))
    {
      pDrawer->screen()->drawArc(m_DebugPts[i], 0, math::pi * 2, 30, graphics::Color(0, 0, 255, 32), graphics::debugDepth);
      pDrawer->screen()->fillSector(m_DebugPts[i], 0, math::pi * 2, 30, graphics::Color(0, 0, 255, 32), graphics::debugDepth);
    }
}

void InformationDisplay::enableRuler(bool doEnable)
{
  if (doEnable)
    m_ruler->AnimateShow();
  else
    m_ruler->AnimateHide();
}

bool InformationDisplay::isRulerEnabled() const
{
  return m_ruler->isVisible();
}

void InformationDisplay::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;

  m_fontDesc.m_size = static_cast<uint32_t>(FONT_SIZE * m_visualScale);

  m_ruler->setFont(gui::Element::EActive, m_fontDesc);
  m_debugLabel->setFont(gui::Element::EActive, m_fontDesc);
}

void InformationDisplay::enableDebugInfo(bool doEnable)
{
  m_debugLabel->setIsVisible(doEnable);
}

void InformationDisplay::setDebugInfo(double frameDuration, int currentScale)
{
  m_frameDuration = frameDuration;
  m_currentScale = currentScale;

  ostringstream out;
  out << "Scale : " << m_currentScale;

  m_debugLabel->setText(out.str());
}

void InformationDisplay::enableMemoryWarning(bool flag)
{
  m_isMemoryWarningEnabled = flag;
}

void InformationDisplay::memoryWarning()
{
  enableMemoryWarning(true);
  m_lastMemoryWarning = my::Timer();
}

void InformationDisplay::drawMemoryWarning(Drawer * drawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

  ostringstream out;
  out << "MemoryWarning : " << m_lastMemoryWarning.ElapsedSeconds() << " sec. ago.";

  drawer->screen()->drawText(m_fontDesc,
                             pos,
                             graphics::EPosAboveRight,
                             out.str(),
                             graphics::debugDepth,
                             false);

  if (m_lastMemoryWarning.ElapsedSeconds() > 5)
    enableMemoryWarning(false);
}

void InformationDisplay::drawPlacemark(Drawer * pDrawer, string const & symbol, m2::PointD const & pt)
{
  pDrawer->screen()->drawDisplayList(m_controller
                                       ->GetDisplayListCache()
                                       ->FindSymbol(symbol.c_str()).get(),
                                     math::Shift(math::Identity<double, 3>(), pt));
}

/*
bool InformationDisplay::s_isLogEnabled = false;
list<string> InformationDisplay::s_log;
size_t InformationDisplay::s_logSize = 10;
my::LogMessageFn InformationDisplay::s_oldLogFn = 0;
threads::Mutex s_logMutex;
WindowHandle * InformationDisplay::s_windowHandle = 0;
size_t s_msgNum = 0;

void InformationDisplay::logMessage(my::LogLevel level, my::SrcPoint const &, string const & msg)
void InformationDisplay::setEmptyCountryName(const char * country)
{
  {
    threads::MutexGuard guard(s_logMutex);
    ostringstream out;
    char const * names[] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };
    out << s_msgNum++ << ":";
    if (level >= 0 && level <= static_cast<int>(ARRAY_SIZE(names)))
      out << names[level];
    else
      out << level;
    out << msg << endl;
    s_log.push_back(out.str());
    while (s_log.size() > s_logSize)
      s_log.pop_front();
  }

  /// call redisplay
  s_windowHandle->invalidate();
  m_countryStatusDisplay->setCountryName(country);
}

void InformationDisplay::enableLog(bool doEnable, WindowHandle * windowHandle)
{
  if (doEnable == s_isLogEnabled)
    return;
  s_isLogEnabled = doEnable;
  if (s_isLogEnabled)
  {
    s_oldLogFn = my::LogMessage;
    my::LogMessage = logMessage;
    s_windowHandle = windowHandle;
  }
  else
  {
    my::LogMessage = s_oldLogFn;
    s_windowHandle = 0;
  }
}

void InformationDisplay::drawLog(Drawer * drawer)
{
  threads::MutexGuard guard(s_logMutex);

  for (list<string>::const_iterator it = s_log.begin(); it != s_log.end(); ++it)
  {
    m_yOffset += 20;

    m2::PointD startPt(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

    graphics::StraightTextElement::Params params;
    params.m_depth = graphics::maxDepth;
    params.m_fontDesc = m_fontDesc;
    params.m_log2vis = false;
    params.m_pivot = startPt;
    params.m_position = graphics::EPosAboveRight;
    params.m_glyphCache = drawer->screen()->glyphCache();
    params.m_logText = strings::MakeUniString(*it);

    graphics::StraightTextElement ste(params);

    drawer->screen()->drawRectangle(
        m2::Inflate(ste.roughBoundRect(), m2::PointD(2, 2)),
        graphics::Color(0, 0, 0, 128),
        graphics::maxDepth - 1
        );

    ste.draw(drawer->screen().get(), math::Identity<double, 3>());
  }
}
*/

void InformationDisplay::enableCompassArrow(bool doEnable)
{
  if (doEnable)
    m_compassArrow->AnimateShow();
  else
    m_compassArrow->AnimateHide();
}

bool InformationDisplay::isCompassArrowEnabled() const
{
  return m_compassArrow->isVisible();
}

void InformationDisplay::setCompassArrowAngle(double angle)
{
  m_compassArrow->SetAngle(angle);
}

void InformationDisplay::enableCountryStatusDisplay(bool doEnable)
{
  m_countryStatusDisplay->setIsVisible(doEnable);
}

void InformationDisplay::setEmptyCountryIndex(storage::TIndex const & idx)
{
  m_countryStatusDisplay->setCountryIndex(idx);
}

void InformationDisplay::setDownloadListener(gui::Button::TOnClickListener l)
{
  m_downloadButton->setOnClickListener(l);
}

void InformationDisplay::enableBenchmarkInfo(bool doEnable)
{
  m_isBenchmarkInfoEnabled = doEnable;
}

bool InformationDisplay::addBenchmarkInfo(string const & name, m2::RectD const & globalRect, double frameDuration)
{
  BenchmarkInfo info;
  info.m_name = name;
  info.m_duration = frameDuration;
  info.m_rect = globalRect;
  m_benchmarkInfo.push_back(info);

/*  string deviceID = GetPlatform().DeviceID();
  transform(deviceID.begin(), deviceID.end(), deviceID.begin(), ::tolower);

  ofstream fout(GetPlatform().WritablePathForFile(deviceID + "_benchmark_results.txt").c_str(), ios::app);
  fout << GetPlatform().DeviceID() << " "
       << VERSION_STRING << " "
       << info.m_name << " "
       << info.m_rect.minX() << " "
       << info.m_rect.minY() << " "
       << info.m_rect.maxX() << " "
       << info.m_rect.maxY() << " "
       << info.m_duration << endl;
*/
  return true;
}

void InformationDisplay::drawBenchmarkInfo(Drawer * pDrawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);
  pDrawer->screen()->drawText(m_fontDesc,
                              pos,
                              graphics::EPosAboveRight,
                              "benchmark info :",
                              graphics::benchmarkDepth,
                              false);

  size_t const count = m_benchmarkInfo.size();
  for (size_t i = (count <= 5 ? 0 : count - 5); i < count; ++i)
  {
    ostringstream out;
    m2::RectD const & r = m_benchmarkInfo[i].m_rect;
    out << "  " << m_benchmarkInfo[i].m_name
                << ", " << "rect: (" << r.minX()
                << ", " << r.minY()
                << ", " << r.maxX()
                << ", " << r.maxY()
                << "), duration : " << m_benchmarkInfo[i].m_duration;
    m_yOffset += 20;
    pos.y += 20;
    pDrawer->screen()->drawText(m_fontDesc,
                                pos,
                                graphics::EPosAboveRight,
                                out.str(),
                                graphics::benchmarkDepth,
                                false);
  }
}

void InformationDisplay::doDraw(Drawer *drawer)
{
  m_yOffset = 0;
  if (m_isDebugPointsEnabled)
    drawDebugPoints(drawer);
  if (m_isMemoryWarningEnabled)
    drawMemoryWarning(drawer);
  if (m_isBenchmarkInfoEnabled)
    drawBenchmarkInfo(drawer);
  //if (s_isLogEnabled)
  //  drawLog(drawer);
}

shared_ptr<CountryStatusDisplay> const & InformationDisplay::countryStatusDisplay() const
{
  return m_countryStatusDisplay;
}

shared_ptr<location::State> const & InformationDisplay::locationState() const
{
  return m_locationState;
}

shared_ptr<Ruler> const & InformationDisplay::ruler() const
{
  return m_ruler;
}
