#include "map/information_display.hpp"
#include "map/country_status_display.hpp"
#include "map/compass_arrow.hpp"
#include "map/framework.hpp"
#include "map/ruler.hpp"
#include "map/alfa_animation_task.hpp"

#include "anim/task.hpp"
#include "anim/controller.hpp"

#include "platform/platform.hpp"

#include "geometry/transformations.hpp"

namespace
{
  static int const RULLER_X_OFFSET = 6;
  static int const RULLER_Y_OFFSET = 42;
  static int const FONT_SIZE = 14;
  static int const COMPASS_X_OFFSET = 8;
  static int const COMPASS_Y_OFFSET = 65;
#ifdef OMIM_OS_ANDROID
  static double const RULLER_X_OFFSET_L = 70.4;
  static double const RULLER_Y_OFFSET_L = 10.5;
  static int const COMPASS_X_OFFSET_L = 18;
  static double const COMPASS_Y_OFFSET_L = 11.4;
#endif
}

InformationDisplay::InformationDisplay(Framework * fw)
  : m_framework(fw)
{
  ///@TODO UVR
  //m_fontDesc.m_color = Color(0x4D, 0x4D, 0x4D, 0xCC);

  InitRuler(fw);
  InitCountryStatusDisplay(fw);
  InitCompassArrow(fw);
  InitLocationState(fw);
  InitDebugLabel();
  InitCopyright(fw);

#ifndef DEBUG
  enableDebugInfo(false);
#endif

  setVisualScale(1);
}

void InformationDisplay::InitRuler(Framework * fw)
{
  ///@TODO UVR
//  Ruler::Params p;

//  p.m_depth = rulerDepth;
//  p.m_position = EPosAboveLeft;
//  p.m_framework = fw;

//  m_ruler.reset(new Ruler(p));
//  m_ruler->setIsVisible(false);
}

void InformationDisplay::InitCountryStatusDisplay(Framework * fw)
{
  ///@TODO UVR
//  CountryStatusDisplay::Params p(fw->GetCountryTree().GetActiveMapLayout());

//  p.m_pivot = m2::PointD(0, 0);
//  p.m_position = EPosCenter;
//  p.m_depth = countryStatusDepth;

//  m_countryStatusDisplay.reset(new CountryStatusDisplay(p));
}

void InformationDisplay::InitCopyright(Framework * fw)
{
  ///@TODO UVR
//  gui::CachedTextView::Params p;

//  p.m_depth = rulerDepth;
//  p.m_position = EPosAboveLeft;
//  p.m_pivot = m2::PointD(0, 0);
//  p.m_text = "Map data © OpenStreetMap";

//  m_copyrightLabel.reset(new gui::CachedTextView(p));
}

void InformationDisplay::InitCompassArrow(Framework * fw)
{
  ///@TODO UVR
//  CompassArrow::Params p;

//  p.m_position = EPosCenter;
//  p.m_depth = compassDepth;
//  p.m_pivot = m2::PointD(0, 0);
//  p.m_framework = fw;

//  m_compassArrow.reset(new CompassArrow(p));
//  m_compassArrow->setIsVisible(false);
}

void InformationDisplay::InitLocationState(Framework * fw)
{
  ///@TODO UVR
//  location::State::Params p;

//  p.m_position = EPosCenter;
//  p.m_depth = locationDepth;
//  p.m_pivot = m2::PointD(0, 0);
//  p.m_locationAreaColor = Color(0x51, 0xA3, 0xDC, 0x46);
//  p.m_framework = fw;

//  m_locationState.reset(new location::State(p));
}

void InformationDisplay::InitDebugLabel()
{
  ///@TODO UVR
//  gui::CachedTextView::Params p;

//  p.m_depth = debugDepth;
//  p.m_position = EPosAboveRight;
//  p.m_pivot = m2::PointD(0, 0);

//  m_debugLabel.reset(new gui::CachedTextView(p));
}

  ///@TODO UVR
//void InformationDisplay::setController(gui::Controller * controller)
//{
//  m_controller = controller;
//  m_controller->AddElement(m_countryStatusDisplay);
//  m_controller->AddElement(m_compassArrow);
//  m_controller->AddElement(m_locationState);
//  m_controller->AddElement(m_ruler);
//  m_controller->AddElement(m_debugLabel);

//  m_controller->AddElement(m_copyrightLabel);
//  shared_ptr<anim::Task> task(new AlfaAnimationTask(1.0, 0.0, 0.15, 3.0, m_framework));
//  task->AddCallback(anim::Task::EEnded, [this] ()
//                                        {
//                                          m_controller->RemoveElement(m_copyrightLabel);
//                                          m_copyrightLabel.reset();
//                                        });

//  m_copyrightLabel->setAnimated([task] ()
//                                {
//                                  AlfaAnimationTask * t = static_cast<AlfaAnimationTask *>(task.get());
//                                  return t->GetCurrentAlfa();
//                                });

//  m_framework->GetAnimController()->AddTask(task);
//}

void InformationDisplay::SetWidgetPivotsByDefault(int screenWidth, int screenHeight)
{

  ///@TODO UVR
//  double rulerOffsX = RULLER_X_OFFSET;
//  double rulerOffsY = RULLER_Y_OFFSET;
//  double compassOffsX = COMPASS_X_OFFSET;
//  double compassOffsY = COMPASS_Y_OFFSET;
//#ifdef OMIM_OS_ANDROID
//  if (GetPlatform().IsTablet())
//  {
//    rulerOffsX = RULLER_X_OFFSET_L;
//    rulerOffsY = RULLER_Y_OFFSET_L;
//    compassOffsX = COMPASS_X_OFFSET_L;
//    compassOffsY = COMPASS_Y_OFFSET_L;
//  }
//#endif

//  double const vs = m_framework->GetVisualScale();
//  m_countryStatusDisplay->setPivot(m2::PointD(rect.Center()));

//  m2::PointD const size = m_compassArrow->GetPixelSize();
//  m_compassArrow->setPivot(m2::PointD(compassOffsX * vs + size.x / 2.0,
//                                      rect.maxY() - compassOffsY * vs - size.y / 2.0));

//  m_ruler->setPivot(m2::PointD(rect.maxX() - rulerOffsX * vs,
//                               rect.maxY() - rulerOffsY * vs));

//  if (m_copyrightLabel)
//  {
//    m_copyrightLabel->setPivot(m2::PointD(rect.maxX() - rulerOffsX * vs,
//                                          rect.maxY() - rulerOffsY * vs));
//  }

//  m_debugLabel->setPivot(m2::PointD(rect.minX() + 10,
//                                    rect.minY() + 50 + 5 * vs));
}

bool InformationDisplay::isCopyrightActive() const
{
  ///@TODO UVR
  return false;//m_copyrightLabel != nullptr;
}

void InformationDisplay::enableCopyright(bool doEnable)
{
  if (m_copyrightLabel != nullptr)
    m_copyrightLabel->setIsVisible(doEnable);
}

void InformationDisplay::enableRuler(bool doEnable)
{
  ///@TODO UVR
//  if (doEnable)
//    m_ruler->AnimateShow();
//  else
//    m_ruler->AnimateHide();
}

bool InformationDisplay::isRulerEnabled() const
{
  ///@TODO UVR
  return false;//m_ruler->isVisible();
}

void InformationDisplay::setVisualScale(double visualScale)
{
  ///@TODO UVR
//  m_fontDesc.m_size = static_cast<uint32_t>(FONT_SIZE * visualScale);

//  m_ruler->setFont(gui::Element::EActive, m_fontDesc);
//  m_debugLabel->setFont(gui::Element::EActive, m_fontDesc);
//  if (m_copyrightLabel)
//    m_copyrightLabel->setFont(gui::Element::EActive, m_fontDesc);
}

void InformationDisplay::enableDebugInfo(bool doEnable)
{
  ///@TODO UVR
  //m_debugLabel->setIsVisible(doEnable);
}

void InformationDisplay::setDebugInfo(double frameDuration, int currentScale)
{
  ostringstream out;
  out << "Scale : " << currentScale;

  ///@TODO UVR
  //m_debugLabel->setText(out.str());
}

void InformationDisplay::enableCompassArrow(bool doEnable)
{
  ///@TODO UVR
//  if (doEnable)
//    m_compassArrow->AnimateShow();
//  else
//    m_compassArrow->AnimateHide();
}

bool InformationDisplay::isCompassArrowEnabled() const
{
  ///@TODO UVR
  return false;//m_compassArrow->isVisible();
}

void InformationDisplay::setCompassArrowAngle(double angle)
{
  ///@TODO UVR
  //m_compassArrow->SetAngle(angle);
}

void InformationDisplay::setEmptyCountryIndex(storage::TIndex const & idx)
{
  ///@TODO UVR
  //m_countryStatusDisplay->SetCountryIndex(idx);
}

shared_ptr<CountryStatusDisplay> const & InformationDisplay::countryStatusDisplay() const
{
  ///@TODO UVR
  return nullptr;//m_countryStatusDisplay;
}

shared_ptr<location::State> const & InformationDisplay::locationState() const
{
  ///@TODO UVR
  return nullptr; //m_locationState;
}

void InformationDisplay::measurementSystemChanged()
{
  ///@TODO UVR
  //m_ruler->setIsDirtyLayout(true);
}

void InformationDisplay::ResetRouteMatchingInfo()
{
  m_locationState->ResetRouteMatchingInfo();
}

void InformationDisplay::SetWidgetPivot(WidgetType widget, m2::PointD const & pivot)
{
  auto const setPivotFn = [](shared_ptr<gui::Element> e, m2::PointD const & point)
  {
    if (e)
      e->setPivot(point);
  };

  switch(widget)
  {
  case WidgetType::Ruler:
    setPivotFn(m_ruler, pivot);
    return;
  case WidgetType::CopyrightLabel:
    setPivotFn(m_copyrightLabel, pivot);
    return;
  case WidgetType::CountryStatusDisplay:
    setPivotFn(m_countryStatusDisplay, pivot);
    return;
  case WidgetType::CompassArrow:
    setPivotFn(m_compassArrow, pivot);
    return;
  case WidgetType::DebugLabel:
    setPivotFn(m_debugLabel, pivot);
    return;
  default:
    ASSERT(false, ());
  }
}

m2::PointD InformationDisplay::GetWidgetSize(WidgetType widget) const
{
  m2::RectD boundRect;
  switch(widget)
  {
  case WidgetType::Ruler:
    if (m_ruler)
      boundRect = m_ruler->GetBoundRect();
    break;
  case WidgetType::CompassArrow:
    if (m_compassArrow)
      return m_compassArrow->GetPixelSize();
  case WidgetType::DebugLabel:
  case WidgetType::CountryStatusDisplay:
  case WidgetType::CopyrightLabel:
  default:
    ASSERT(false, ());
  }
  return m2::PointD(boundRect.SizeX(), boundRect.SizeY());
}
