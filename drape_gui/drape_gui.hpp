#pragma once

#include "../std/function.hpp"
#include "../std/unique_ptr.hpp"

class ScreenBase;

namespace gui
{
class RulerHelper;
class CountryStatusHelper;

class DrapeGui
{
public:

  typedef function<double  ()> TScaleFactorFn;
  typedef function<int (ScreenBase const &)> TGeneralizationLevelFn;

  static DrapeGui & Instance();
  static RulerHelper & GetRulerHelper();
  static CountryStatusHelper & GetCountryStatusHelper();

  void Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn);
  double GetScaleFactor();
  int GetGeneralization(ScreenBase const & screen);

private:
  RulerHelper & GetRulerHelperImpl();
  CountryStatusHelper & GetCountryStatusHelperImpl();

private:
  struct Impl;
  unique_ptr<Impl> m_impl;
};

}
