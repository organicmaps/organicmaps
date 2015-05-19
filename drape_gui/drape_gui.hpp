#pragma once

#include "skin.hpp"
#include "compass.hpp"
#include "country_status.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "drape/pointers.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"

class ScreenBase;

namespace gui
{

class RulerHelper;
class CountryStatusHelper;

class DrapeGui
{
public:
  using TRecacheSlot = function<void (Skin::ElementName)>;
  using TScaleFactorFn = function<double ()>;
  using TGeneralizationLevelFn = function<int (ScreenBase const &)>;
  using TLocalizeStringFn = function<string (string const &)>;

  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();
  static CountryStatusHelper & GetCountryStatusHelper();

  static dp::FontDecl const & GetGuiTextFont();

  void Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn);
  void SetLocalizator(TLocalizeStringFn const & fn);
  void Destroy();

  double GetScaleFactor();
  int GetGeneralization(ScreenBase const & screen);
  string GetLocalizedString(string const & stringID) const;

  void SetRecacheSlot(TRecacheSlot const & fn);
  void EmitRecacheSignal(Skin::ElementName elements);
  void ClearRecacheSlot();

  bool IsCopyrightActive() const { return m_isCopyrightActive; }
  void DeactivateCopyright() { m_isCopyrightActive = false; }

  void ConnectOnCompassTappedHandler(Shape::TTapHandler const & handler);
  void ConnectOnButtonPressedHandler(CountryStatusHelper::EButtonType buttonType,
                                     Shape::TTapHandler const & handler);
  void CallOnCompassTappedHandler();
  void CallOnButtonPressedHandler(CountryStatusHelper::EButtonType buttonType);

private:
  RulerHelper & GetRulerHelperImpl();
  CountryStatusHelper & GetCountryStatusHelperImpl();

private:
  struct Impl;
  unique_ptr<Impl> m_impl;
  bool m_isCopyrightActive = true;

  Shape::TTapHandler m_onCompassTappedHandler;
  CountryStatus::TButtonHandlers m_buttonHandlers;
};

}
