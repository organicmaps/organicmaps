#include "drape_gui.hpp"

#include "drape_frontend/color_constants.hpp"

namespace gui
{
df::ColorConstant const kGuiTextColor = "GuiText";

DrapeGui & DrapeGui::Instance()
{
  static DrapeGui s_gui;
  return s_gui;
}

RulerHelper & DrapeGui::GetRulerHelper()
{
  return Instance().m_rulerHelper;
}

dp::FontDecl DrapeGui::GetGuiTextFont()
{
  return {df::GetColorConstant(kGuiTextColor), 14};
}

void DrapeGui::SetSurfaceSize(m2::PointF const & size)
{
  std::lock_guard lock(m_paramsMutex);
  m_surfaceSize = size;
}

m2::PointF DrapeGui::GetSurfaceSize() const
{
  std::lock_guard lock(m_paramsMutex);
  return m_surfaceSize;
}

void DrapeGui::SetVisibleViewport(m2::RectD const & rect)
{
  std::lock_guard lock(m_paramsMutex);
  m_visibleViewport = rect;
}

m2::RectD DrapeGui::GetVisibleViewport() const
{
  std::lock_guard lock(m_paramsMutex);

  if (m_visibleViewport.IsValid())
    return m_visibleViewport;
  else
  {
    // Better than assign in SetSurfaceSize.
    return {0, 0, m_surfaceSize.x, m_surfaceSize.y};
  }
}

void DrapeGui::ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler)
{
  m_onCompassTappedHandler = handler;
}

void DrapeGui::CallOnCompassTappedHandler()
{
  if (m_onCompassTappedHandler != nullptr)
    m_onCompassTappedHandler();
}
}  // namespace gui
