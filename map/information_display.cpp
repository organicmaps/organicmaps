#include "map/information_display.hpp"
#include "map/compass_arrow.hpp"
#include "map/framework.hpp"
#include "map/alfa_animation_task.hpp"

#include "anim/task.hpp"
#include "anim/controller.hpp"

#include "platform/platform.hpp"

#include "geometry/transformations.hpp"

void InformationDisplay::InitLocationState()
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
