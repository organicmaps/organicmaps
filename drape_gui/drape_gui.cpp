#include "country_status_helper.hpp"
#include "drape_gui.hpp"
#include "ruler_helper.hpp"

#include "base/assert.hpp"

namespace gui
{

void StorageAccessor::SetStatusChangedCallback(TSlotFn const & fn)
{
  m_statusChanged = fn;
}

struct DrapeGui::Impl
{
  DrapeGui::TScaleFactorFn m_scaleFn;
  DrapeGui::TGeneralizationLevelFn m_gnLvlFn;
  DrapeGui::TLocalizeStringFn m_localizeFn;

  DrapeGui::TRecacheSlot m_recacheSlot;

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

dp::FontDecl const & DrapeGui::GetGuiTextFont()
{
  static dp::FontDecl font(dp::Color(0x4D, 0x4D, 0x4D, 0xDD), 7 * DrapeGui::Instance().GetScaleFactor());
  return font;
}

void DrapeGui::Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn)
{
  ASSERT(m_impl == nullptr, ());
  m_impl.reset(new DrapeGui::Impl());
  m_impl->m_scaleFn = scaleFn;
  m_impl->m_gnLvlFn = gnLvlFn;
}

void DrapeGui::Destroy()
{
  ASSERT(m_impl != nullptr, ());
  m_impl.reset();
}

void DrapeGui::SetLocalizator(const DrapeGui::TLocalizeStringFn & fn)
{
  ASSERT(m_impl != nullptr, ());
  m_impl->m_localizeFn = fn;
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

void DrapeGui::SetRecacheSlot(DrapeGui::TRecacheSlot const  & fn)
{
  ASSERT(m_impl != nullptr, ());
  m_impl->m_recacheSlot = fn;
}

void DrapeGui::EmitRecacheSignal(Skin::ElementName elements)
{
  ASSERT(m_impl != nullptr, ());
  if (m_impl->m_recacheSlot)
    m_impl->m_recacheSlot(elements);
}

void DrapeGui::ClearRecacheSlot()
{
  SetRecacheSlot(TRecacheSlot());
}

string DrapeGui::GetLocalizedString(string const & stringID) const
{
  ASSERT(m_impl != nullptr, ());
  ASSERT(m_impl->m_localizeFn != nullptr, ());
  return m_impl->m_localizeFn(stringID);
}

void DrapeGui::SetStorageAccessor(ref_ptr<StorageAccessor> accessor)
{
  ASSERT(m_impl != nullptr, ());
  accessor->SetStatusChangedCallback([this]
  {
    EmitRecacheSignal(Skin::CountryStatus);
  });

  CountryStatusHelper & cntHelpet = GetCountryStatusHelperImpl();
  cntHelpet.SetStorageAccessor(accessor);
}

void DrapeGui::SetCountryIndex(storage::TIndex const & index)
{
  GetCountryStatusHelperImpl().SetCountryIndex(index);
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
