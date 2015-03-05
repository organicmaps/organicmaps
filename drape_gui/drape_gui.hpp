#pragma once

#include "../std/function.hpp"
#include "../std/unique_ptr.hpp"

class ScreenBase;

namespace gui
{

class DrapeGui
{
public:

  typedef function<double  ()> TScaleFactorFn;
  typedef function<int (ScreenBase const &)> TGeneralizationLevelFn;

  static DrapeGui & Instance();

  void Init(TScaleFactorFn const & scaleFn, TGeneralizationLevelFn const & gnLvlFn);
  double GetScaleFactor();
  int GetGeneralization(ScreenBase const & screen);

private:
  struct Impl;
  unique_ptr<Impl> m_impl;

};

}
