#include "drape_gui.hpp"
#include "ruler_helper.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/assert.hpp"

namespace gui
{

df::ColorConstant const kGuiTextColor = "GuiText";

struct DrapeGui::Impl
{
  DrapeGui::TLocalizeStringFn m_localizeFn;
  RulerHelper m_rulerHelper;
};

DrapeGui::DrapeGui()
  : m_impl(new Impl())
{}

DrapeGui & DrapeGui::Instance()
{
  static DrapeGui s_gui;
  if (!s_gui.m_impl)
    s_gui.m_impl.reset(new Impl());

  return s_gui;
}

RulerHelper & DrapeGui::GetRulerHelper()
{
  return Instance().GetRulerHelperImpl();
}

dp::FontDecl DrapeGui::GetGuiTextFont()
{
  return dp::FontDecl(df::GetColorConstant(kGuiTextColor), 14);
}

void DrapeGui::Destroy()
{
  ASSERT(m_impl != nullptr, ());
  m_impl.reset();
}

void DrapeGui::SetSurfaceSize(m2::PointF const & size)
{
  lock_guard<mutex> lock(m_surfaceSizeMutex);
  m_surfaceSize = size;
}

m2::PointF DrapeGui::GetSurfaceSize() const
{
  lock_guard<mutex> lock(m_surfaceSizeMutex);
  return m_surfaceSize;
}

void DrapeGui::SetLocalizator(const DrapeGui::TLocalizeStringFn & fn)
{
  ASSERT(m_impl != nullptr, ());
  m_impl->m_localizeFn = fn;
}

string DrapeGui::GetLocalizedString(string const & stringID) const
{
  ASSERT(m_impl != nullptr, ());
  ASSERT(m_impl->m_localizeFn != nullptr, ());
  return m_impl->m_localizeFn(stringID);
}

RulerHelper & DrapeGui::GetRulerHelperImpl()
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_rulerHelper;
}

void DrapeGui::ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler)
{
  m_onCompassTappedHandler = handler;
}

void DrapeGui::CallOnCompassTappedHandler()
{
  if(m_onCompassTappedHandler != nullptr)
    m_onCompassTappedHandler();
}

} // namespace gui
