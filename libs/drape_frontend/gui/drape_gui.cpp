#include "drape_gui.hpp"
#include "ruler_helper.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

#include "base/assert.hpp"

namespace gui
{
df::ColorConstant const kGuiTextColor = "GuiText";

struct DrapeGui::Impl
{
  RulerHelper m_rulerHelper;
  speed_limit::SpeedLimit m_speedLimit;
};

DrapeGui::DrapeGui() : m_impl(std::make_unique<Impl>()), m_surfaceSize() {}

DrapeGui & DrapeGui::Instance()
{
  static DrapeGui s_gui;
  if (!s_gui.m_impl)
    s_gui.m_impl = std::make_unique<Impl>();

  return s_gui;
}

RulerHelper & DrapeGui::GetRulerHelper()
{
  return Instance().GetRulerHelperImpl();
}

speed_limit::SpeedLimit & DrapeGui::GetSpeedLimit()
{
  return Instance().GetSpeedLimitImpl();
}

dp::FontDecl DrapeGui::GetGuiTextFont()
{
  return {df::GetColorConstant(kGuiTextColor), 14};
}

void DrapeGui::Destroy()
{
  ASSERT(m_impl != nullptr, ());
  m_impl.reset();
}

void DrapeGui::SetSurfaceSize(m2::PointF const & size)
{
  std::lock_guard lock(m_surfaceSizeMutex);
  m_surfaceSize = size;
}

m2::PointF DrapeGui::GetSurfaceSize() const
{
  std::lock_guard lock(m_surfaceSizeMutex);
  return m_surfaceSize;
}

RulerHelper & DrapeGui::GetRulerHelperImpl() const
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_rulerHelper;
}

speed_limit::SpeedLimit & DrapeGui::GetSpeedLimitImpl() const
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_speedLimit;
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
