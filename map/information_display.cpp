#include "information_display.hpp"
#include "drawer_yg.hpp"
#include "country_status_display.hpp"
#include "compass_arrow.hpp"
#include "framework.hpp"

#include "../indexer/mercator.hpp"

#include "../gui/controller.hpp"
#include "../gui/button.hpp"

#include "../yg/defines.hpp"
#include "../yg/skin.hpp"
#include "../yg/pen_info.hpp"
#include "../yg/straight_text_element.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/mutex.hpp"
#include "../base/macros.hpp"

#include "../geometry/transformations.hpp"

#include "../std/fstream.hpp"
#include "../std/iomanip.hpp"
#include "../std/target_os.hpp"

InformationDisplay::InformationDisplay(Framework * framework)
  : m_ruler(Ruler::Params()),
    m_bottomShift(0)
{
  CountryStatusDisplay::Params p;

  p.m_pivot = m2::PointD(0, 0);
  p.m_position = yg::EPosCenter;
  p.m_depth = yg::maxDepth;
  p.m_storage = &framework->Storage();

  m_countryStatusDisplay.reset(new CountryStatusDisplay(p));

  CompassArrow::Params cap;

  cap.m_position = yg::EPosCenter;
  cap.m_depth = yg::maxDepth;
  cap.m_arrowHeight = 60;
  cap.m_arrowWidth = 20;
  cap.m_pivot = m2::PointD(10 + cap.m_arrowHeight / 2, 10 + cap.m_arrowHeight / 2);
  cap.m_northColor = yg::Color(255, 0, 0, 255);
  cap.m_southColor = yg::Color(255, 255, 255, 255);
  cap.m_framework = framework;

  m_compassArrow.reset(new CompassArrow(cap));

  location::State::Params lsp;

  lsp.m_position = yg::EPosCenter;
  lsp.m_depth = yg::maxDepth;
  lsp.m_pivot = m2::PointD(0, 0);
  lsp.m_compassAreaColor = yg::Color(255, 255, 255, 192);
  lsp.m_compassBorderColor = yg::Color(255, 255, 255, 96);
  lsp.m_locationAreaColor = yg::Color(0, 0, 255, 32);
  lsp.m_locationBorderColor = yg::Color(0, 0, 255, 32);
  lsp.m_framework = framework;

  m_locationState.reset(new location::State(lsp));

  enableDebugPoints(false);
  enableRuler(false);
  enableCenter(false);
  enableDebugInfo(false);
  enableMemoryWarning(false);
  enableBenchmarkInfo(false);
  enableCountryStatusDisplay(false);
  enableCompassArrow(false);

  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    m_DebugPts[i] = m2::PointD(0, 0);

  m_fontDesc = yg::FontDesc(12);
}

void InformationDisplay::setController(gui::Controller *controller)
{
  m_controller = controller;
  m_controller->AddElement(m_countryStatusDisplay);
  m_controller->AddElement(m_compassArrow);
  m_controller->AddElement(m_locationState);
}

void InformationDisplay::setScreen(ScreenBase const & screen)
{
  m_screen = screen;
  m_ruler.setScreen(screen);

  if (m_countryStatusDisplay->isVisible())
  {
    m2::RectD pxRect = m_screen.PixelRect();
    m2::PointD pt = m2::PointD(pxRect.SizeX() / 2, pxRect.SizeY() / 2) - m2::PointD(0, m_bottomShift * m_visualScale);
    m_countryStatusDisplay->setPivot(pt);
  }
}

void InformationDisplay::setBottomShift(double bottomShift)
{
  m_bottomShift = bottomShift;
}

void InformationDisplay::setDisplayRect(m2::RectI const & rect)
{
  m_displayRect = rect;
}

void InformationDisplay::enableDebugPoints(bool doEnable)
{
  m_isDebugPointsEnabled = doEnable;
}

void InformationDisplay::setDebugPoint(int pos, m2::PointD const & pt)
{
  m_DebugPts[pos] = pt;
}

void InformationDisplay::drawDebugPoints(DrawerYG * pDrawer)
{
  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    if (m_DebugPts[i] != m2::PointD(0, 0))
    {
    pDrawer->screen()->drawArc(m_DebugPts[i], 0, math::pi * 2, 30, yg::Color(0, 0, 255, 32), yg::maxDepth);
    pDrawer->screen()->fillSector(m_DebugPts[i], 0, math::pi * 2, 30, yg::Color(0, 0, 255, 32), yg::maxDepth);
  }
}

void InformationDisplay::enableRuler(bool doEnable)
{
  m_isRulerEnabled = doEnable;
}

void InformationDisplay::setRulerParams(unsigned pxMinWidth, double metresMinWidth, double metresMaxWidth)
{
  m_ruler.setMinPxWidth(pxMinWidth);
  m_ruler.setMinUnitsWidth(metresMinWidth);
  m_ruler.setMaxUnitsWidth(metresMaxWidth);
}

void InformationDisplay::drawRuler(DrawerYG * pDrawer)
{
  yg::FontDesc rulerFont = m_fontDesc;
  rulerFont.m_color = yg::Color(0x44, 0x44, 0x44, 0xD9);

  m_ruler.setFontDesc(rulerFont);
  m_ruler.setVisualScale(m_visualScale);

#if defined(OMIM_OS_IPHONE)
  m2::PointD pivot(m2::PointD(m_displayRect.maxX(),
                              m_displayRect.maxY() - 20 * m_visualScale)
                 + m2::PointD(-10 * m_visualScale, - 10 * m_visualScale));
  m_ruler.setPosition(yg::EPosAboveLeft);
#elif defined(OMIM_OS_ANDROID)
  m2::PointD pivot(m2::PointD(m_displayRect.maxX(),
                              m_displayRect.maxY() - 20 * m_visualScale)
                 + m2::PointD(-10 * m_visualScale, - 10 * m_visualScale));
  m_ruler.setPosition(yg::EPosAboveLeft);
#else
  m2::PointD pivot(m2::PointD(m_displayRect.minX(),
                              m_displayRect.maxY() - m_bottomShift * m_visualScale)
                 + m2::PointD(10 * m_visualScale, -10 * m_visualScale));

  m_ruler.setPosition(yg::EPosAboveRight);
#endif
  m_ruler.setPivot(pivot);
  m_ruler.update();

  m_ruler.draw(pDrawer->screen().get(), math::Identity<double, 3>());
}

void InformationDisplay::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;

  m_fontDesc.m_size = static_cast<uint32_t>(12 * visualScale);
}

void InformationDisplay::enableCenter(bool doEnable)
{
  m_isCenterEnabled = doEnable;
}

void InformationDisplay::setCenter(m2::PointD const & pt)
{
  m_centerPtLonLat = pt;
}

void InformationDisplay::drawCenter(DrawerYG * drawer)
{
  ostringstream out;

  out << "(" << fixed << setprecision(4) << m_centerPtLonLat.y << ", "
             << fixed << setprecision(4) << setw(8) << m_centerPtLonLat.x << ")";

  yg::StraightTextElement::Params params;

  params.m_depth = yg::maxDepth;
  params.m_fontDesc = m_fontDesc;
  params.m_fontDesc.m_color = yg::Color(0x44, 0x44, 0x44, 0xD9);
  params.m_log2vis = false;

#if defined(OMIM_OS_IPHONE)
  params.m_pivot = m2::PointD(m_displayRect.maxX() - 10 * m_visualScale,
                              m_displayRect.maxY() - 10 * m_visualScale);
  params.m_position = yg::EPosAboveLeft;
#elif defined(OMIM_OS_ANDROID)
  params.m_pivot = m2::PointD(m_displayRect.maxX() - 10 * m_visualScale,
                              m_displayRect.maxY() - 10 * m_visualScale);
  params.m_position = yg::EPosAboveLeft;
#else
  params.m_pivot = m2::PointD(m_displayRect.maxX() - 10 * m_visualScale,
                              m_displayRect.maxY() - (/*m_bottomShift*/ + 14) * m_visualScale - 5);
  params.m_position = yg::EPosAboveLeft;
#endif

  params.m_glyphCache = drawer->screen()->glyphCache();
  params.m_logText = strings::MakeUniString(out.str());

  yg::StraightTextElement ste(params);

//  m2::RectD bgRect = m2::Inflate(ste.roughBoundRect(), 5.0, 5.0);

//  drawer->screen()->drawRectangle(
//        bgRect,
//        yg::Color(187, 187, 187, 128),
//        yg::maxDepth - 1);

  ste.draw(drawer->screen().get(), math::Identity<double, 3>());
}

void InformationDisplay::enableDebugInfo(bool doEnable)
{
  m_isDebugInfoEnabled = doEnable;
}

void InformationDisplay::setDebugInfo(double frameDuration, int currentScale)
{
  m_frameDuration = frameDuration;
  m_currentScale = currentScale;
}

void InformationDisplay::drawDebugInfo(DrawerYG * drawer)
{
  ostringstream out;
  out << "Scale : " << m_currentScale;

/*  out << "SPF: " << m_frameDuration;
  if (m_frameDuration == 0.0)
    out << " FPS: inf";
  else
    out << " FPS: " << 1.0 / m_frameDuration;*/

  m_yOffset += 20;

  m2::PointD pos = m2::PointD(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset + 5 * m_visualScale);

  drawer->screen()->drawText(m_fontDesc,
                             pos,
                             yg::EPosAboveRight,
                             out.str(),
                             yg::maxDepth,
                             false);
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

void InformationDisplay::drawMemoryWarning(DrawerYG * drawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

  ostringstream out;
  out << "MemoryWarning : " << m_lastMemoryWarning.ElapsedSeconds() << " sec. ago.";

  drawer->screen()->drawText(m_fontDesc,
                             pos,
                             yg::EPosAboveRight,
                             out.str(),
                             yg::maxDepth,
                             false);

  if (m_lastMemoryWarning.ElapsedSeconds() > 5)
    enableMemoryWarning(false);
}

void InformationDisplay::drawPlacemark(DrawerYG * pDrawer, string const & symbol, m2::PointD const & pt)
{
  pDrawer->drawSymbol(pt, symbol, yg::EPosAbove, yg::maxDepth);
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

void InformationDisplay::drawLog(DrawerYG * drawer)
{
  threads::MutexGuard guard(s_logMutex);

  for (list<string>::const_iterator it = s_log.begin(); it != s_log.end(); ++it)
  {
    m_yOffset += 20;

    m2::PointD startPt(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

    yg::StraightTextElement::Params params;
    params.m_depth = yg::maxDepth;
    params.m_fontDesc = m_fontDesc;
    params.m_log2vis = false;
    params.m_pivot = startPt;
    params.m_position = yg::EPosAboveRight;
    params.m_glyphCache = drawer->screen()->glyphCache();
    params.m_logText = strings::MakeUniString(*it);

    yg::StraightTextElement ste(params);

    drawer->screen()->drawRectangle(
        m2::Inflate(ste.roughBoundRect(), m2::PointD(2, 2)),
        yg::Color(0, 0, 0, 128),
        yg::maxDepth - 1
        );

    ste.draw(drawer->screen().get(), math::Identity<double, 3>());
  }
}
*/

void InformationDisplay::enableCompassArrow(bool doEnable)
{
  m_compassArrow->setIsVisible(doEnable);
}

void InformationDisplay::setCompassArrowAngle(double angle)
{
  m_compassArrow->setAngle(angle);
}

void InformationDisplay::enableCountryStatusDisplay(bool doEnable)
{
  m_countryStatusDisplay->setIsVisible(doEnable);
}

void InformationDisplay::setEmptyCountryName(string const & country)
{
  m_countryStatusDisplay->setCountryName(country);
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

void InformationDisplay::drawBenchmarkInfo(DrawerYG * pDrawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);
  pDrawer->screen()->drawText(m_fontDesc,
                              pos,
                              yg::EPosAboveRight,
                              "benchmark info :",
                              yg::maxDepth,
                              false);

  for (unsigned i = max(0, (int)m_benchmarkInfo.size() - 5); i < m_benchmarkInfo.size(); ++i)
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
                                yg::EPosAboveRight,
                                out.str(),
                                yg::maxDepth,
                                false
                                );
  }

}

void InformationDisplay::setupRuler()
{
  m_ruler.setup();
}

void InformationDisplay::doDraw(DrawerYG *drawer)
{
  m_yOffset = 0;
  if (m_isDebugPointsEnabled)
    drawDebugPoints(drawer);
  if (m_isRulerEnabled)
    drawRuler(drawer);
  if (m_isCenterEnabled)
    drawCenter(drawer);
  if (m_isDebugInfoEnabled)
    drawDebugInfo(drawer);
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
