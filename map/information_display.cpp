#include "information_display.hpp"
#include "drawer.hpp"
#include "country_status_display.hpp"
#include "compass_arrow.hpp"
#include "framework.hpp"
#include "ruler.hpp"
#include "alfa_animation_task.hpp"

#include "../anim/task.hpp"
#include "../anim/controller.hpp"

#include "../gui/controller.hpp"
#include "../gui/button.hpp"
#include "../gui/cached_text_view.hpp"

#include "../graphics/defines.hpp"
#include "../graphics/depth_constants.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/transformations.hpp"


using namespace graphics;

namespace
{
  static int const RULLER_X_OFFSET = 6;
  static int const RULLER_Y_OFFSET = 42;
  static int const FONT_SIZE = 14;
  static int const COMPASS_X_OFFSET = 27;
  static int const COMPASS_Y_OFFSET = 57;
}

InformationDisplay::InformationDisplay(Framework * fw)
  : m_visualScale(1)
{
  m_fontDesc.m_color = Color(0x4D, 0x4D, 0x4D, 0xCC);

  InitRuler(fw);
  InitCountryStatusDisplay(fw);
  InitCompassArrow(fw);
  InitLocationState(fw);
  InitDebugLabel();
  InitCopyright(fw);

  enableDebugPoints(false);
  enableDebugInfo(false);
  enableMemoryWarning(false);
  enableBenchmarkInfo(false);
  enableCountryStatusDisplay(false);

  m_compassArrow->setIsVisible(false);
  m_ruler->setIsVisible(false);

  for (size_t i = 0; i < ARRAY_SIZE(m_DebugPts); ++i)
    m_DebugPts[i] = m2::PointD(0, 0);

  setVisualScale(m_visualScale);
}

void InformationDisplay::InitRuler(Framework * fw)
{
  Ruler::Params p;

  p.m_depth = rulerDepth;
  p.m_position = EPosAboveLeft;
  p.m_framework = fw;

  m_ruler.reset(new Ruler(p));
}

void InformationDisplay::InitCountryStatusDisplay(Framework * fw)
{
  CountryStatusDisplay::Params p;

  p.m_pivot = m2::PointD(0, 0);
  p.m_position = EPosCenter;
  p.m_depth = countryStatusDepth;
  p.m_storage = &fw->Storage();

  m_countryStatusDisplay.reset(new CountryStatusDisplay(p));
}

void InformationDisplay::InitCopyright(Framework * fw)
{
  gui::CachedTextView::Params p;

  p.m_depth = rulerDepth;
  p.m_position = EPosAboveLeft;
  p.m_pivot = m2::PointD(0, 0);
  p.m_text = "Map data © OpenStreetMap";

  m_copyrightLabel.reset(new gui::CachedTextView(p));

  shared_ptr<anim::Task> task(new AlfaAnimationTask(1.0, 0.0, 0.15, 3.0, fw));
  task->AddCallback(anim::Task::EEnded, [this]()
                                        {
                                          m_controller->RemoveElement(m_copyrightLabel);
                                          m_copyrightLabel.reset();
                                        });

  m_copyrightLabel->setAnimated([task]()
                                {
                                  AlfaAnimationTask * t = static_cast<AlfaAnimationTask *>(task.get());
                                  return t->GetCurrentAlfa();
                                });

  fw->GetAnimController()->AddTask(task);
}

void InformationDisplay::InitCompassArrow(Framework * fw)
{
  CompassArrow::Params p;

  p.m_position = EPosCenter;
  p.m_depth = compassDepth;
  p.m_pivot = m2::PointD(0, 0);
  p.m_framework = fw;

  m_compassArrow.reset(new CompassArrow(p));
}

void InformationDisplay::InitLocationState(Framework * fw)
{
  location::State::Params p;

  p.m_position = EPosCenter;
  p.m_depth = locationDepth;
  p.m_pivot = m2::PointD(0, 0);
  p.m_locationAreaColor = Color(0x51, 0xA3, 0xDC, 0x46);
  p.m_framework = fw;

  m_locationState.reset(new location::State(p));
}

void InformationDisplay::InitDebugLabel()
{
  gui::CachedTextView::Params p;

  p.m_depth = debugDepth;
  p.m_position = EPosAboveRight;
  p.m_pivot = m2::PointD(0, 0);

  m_debugLabel.reset(new gui::CachedTextView(p));
}

void InformationDisplay::setController(gui::Controller * controller)
{
  m_controller = controller;
  m_controller->AddElement(m_countryStatusDisplay);
  m_controller->AddElement(m_compassArrow);
  m_controller->AddElement(m_locationState);
  m_controller->AddElement(m_ruler);
  m_controller->AddElement(m_debugLabel);
  m_controller->AddElement(m_copyrightLabel);
}

void InformationDisplay::setDisplayRect(m2::RectI const & rect)
{
  m_displayRect = rect;

  m_countryStatusDisplay->setPivot(m2::PointD(rect.Center()));

  m2::PointD const size = m_compassArrow->GetPixelSize();
  m_compassArrow->setPivot(m2::PointD(COMPASS_X_OFFSET * m_visualScale + size.x / 2.0,
                                      rect.maxY() - COMPASS_Y_OFFSET * m_visualScale - size.y / 2.0));

  m_ruler->setPivot(m2::PointD(rect.maxX() - RULLER_X_OFFSET * m_visualScale,
                               rect.maxY() - RULLER_Y_OFFSET * m_visualScale));

  if (m_copyrightLabel)
  {
    m_copyrightLabel->setPivot(m2::PointD(rect.maxX() - RULLER_X_OFFSET * m_visualScale,
                                          rect.maxY() - RULLER_Y_OFFSET * m_visualScale));
  }

  m_debugLabel->setPivot(m2::PointD(rect.minX() + 10,
                                    rect.minY() + 50 + 5 * m_visualScale));
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
  for (size_t i = 0; i < ARRAY_SIZE(m_DebugPts); ++i)
    if (m_DebugPts[i] != m2::PointD(0, 0))
    {
      pDrawer->screen()->drawArc(m_DebugPts[i], 0, math::pi * 2, 30, Color(0, 0, 255, 32), debugDepth);
      pDrawer->screen()->fillSector(m_DebugPts[i], 0, math::pi * 2, 30, Color(0, 0, 255, 32), debugDepth);
    }
}

bool InformationDisplay::isCopyrightActive() const
{
  return m_copyrightLabel != nullptr;
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
  m_copyrightLabel->setFont(gui::Element::EActive, m_fontDesc);
  m_debugLabel->setFont(gui::Element::EActive, m_fontDesc);
}

void InformationDisplay::enableDebugInfo(bool doEnable)
{
  m_debugLabel->setIsVisible(doEnable);
}

void InformationDisplay::setDebugInfo(double frameDuration, int currentScale)
{
  ostringstream out;
  out << "Scale : " << currentScale;

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
                             EPosAboveRight,
                             out.str(),
                             debugDepth,
                             false);

  if (m_lastMemoryWarning.ElapsedSeconds() > 5)
    enableMemoryWarning(false);
}

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

void InformationDisplay::setDownloadListener(gui::Button::TOnClickListener const & l)
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

  return true;
}

void InformationDisplay::drawBenchmarkInfo(Drawer * pDrawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);
  pDrawer->screen()->drawText(m_fontDesc,
                              pos,
                              EPosAboveRight,
                              "benchmark info :",
                              benchmarkDepth,
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
                                EPosAboveRight,
                                out.str(),
                                benchmarkDepth,
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
}

shared_ptr<CountryStatusDisplay> const & InformationDisplay::countryStatusDisplay() const
{
  return m_countryStatusDisplay;
}

shared_ptr<location::State> const & InformationDisplay::locationState() const
{
  return m_locationState;
}

void InformationDisplay::measurementSystemChanged()
{
  m_ruler->setIsDirtyLayout(true);
}
