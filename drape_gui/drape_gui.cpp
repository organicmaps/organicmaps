#include "drape_gui.hpp"
#include "ruler_helper.hpp"
#include "country_status_helper.hpp"

#include "../base/assert.hpp"

namespace gui
{

struct DrapeGui::Impl
{
  DrapeGui::TScaleFactorFn m_scaleFn;
  DrapeGui::TGeneralizationLevelFn m_gnLvlFn;
  RulerHelper m_rulerHelper;
  CountryStatusHelper m_countryHelper;
};

DrapeGui & DrapeGui::Instance()
{
  static DrapeGui s_gui;
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

void DrapeGui::Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn)
{
  ASSERT(m_impl == nullptr, ());
  m_impl.reset(new DrapeGui::Impl());
  m_impl->m_scaleFn = scaleFn;
  m_impl->m_gnLvlFn = gnLvlFn;
}

double DrapeGui::GetScaleFactor()
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_scaleFn();
}

int DrapeGui::GetGeneralization(ScreenBase const & screen)
{
  ASSERT(m_impl != nullptr, ());
  return m_impl->m_gnLvlFn(screen);
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
}
