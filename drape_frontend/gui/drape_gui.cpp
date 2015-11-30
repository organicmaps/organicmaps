#include "country_status_helper.hpp"
#include "drape_gui.hpp"
#include "ruler_helper.hpp"

#include "drape_frontend/visual_params.hpp"

#include "base/assert.hpp"

namespace gui
{

struct DrapeGui::Impl
{
  DrapeGui::TLocalizeStringFn m_localizeFn;
  DrapeGui::TRecacheCountryStatusSlot m_recacheSlot;

  RulerHelper m_rulerHelper;
  CountryStatusHelper m_countryHelper;
};

DrapeGui::DrapeGui()
  : m_impl(new Impl())
{
}

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

CountryStatusHelper & DrapeGui::GetCountryStatusHelper()
{
  return Instance().GetCountryStatusHelperImpl();
}

dp::FontDecl const & DrapeGui::GetGuiTextFont()
{
  static dp::FontDecl font(dp::Color(0x4D, 0x4D, 0x4D, 0xDD), 14);
  return font;
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

void DrapeGui::SetRecacheCountryStatusSlot(TRecacheCountryStatusSlot const  & fn)
{
  ASSERT(m_impl != nullptr, ());
  m_impl->m_recacheSlot = fn;
}

void DrapeGui::EmitRecacheCountryStatusSignal()
{
  ASSERT(m_impl != nullptr, ());
  if (m_impl->m_recacheSlot)
    m_impl->m_recacheSlot();
}

void DrapeGui::ClearRecacheCountryStatusSlot()
{
  SetRecacheCountryStatusSlot(TRecacheCountryStatusSlot());
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

CountryStatusHelper & DrapeGui::GetCountryStatusHelperImpl()
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_countryHelper;
}

void DrapeGui::ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler)
{
  m_onCompassTappedHandler = handler;
}

void DrapeGui::ConnectOnButtonPressedHandler(CountryStatusHelper::EButtonType buttonType,
                                             Shape::TTapHandler const & handler)
{
  m_buttonHandlers[buttonType] = handler;
}

void DrapeGui::CallOnCompassTappedHandler()
{
  if(m_onCompassTappedHandler != nullptr)
    m_onCompassTappedHandler();
}

void DrapeGui::CallOnButtonPressedHandler(CountryStatusHelper::EButtonType buttonType)
{
  auto it = m_buttonHandlers.find(buttonType);
  if (it != m_buttonHandlers.end() && it->second != nullptr)
    it->second();
}

}
