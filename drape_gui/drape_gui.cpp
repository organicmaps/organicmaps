#include "drape_gui.hpp"

#include "../base/assert.hpp"

namespace gui
{

struct DrapeGui::Impl
{
  DrapeGui::TScaleFactorFn m_scaleFn;
  DrapeGui::TGeneralizationLevelFn m_gnLvlFn;
};

DrapeGui & DrapeGui::Instance()
{
  static DrapeGui s_gui;
  return s_gui;
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

}
