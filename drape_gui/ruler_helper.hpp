#pragma once

#include "std/string.hpp"

class ScreenBase;

namespace gui
{

class RulerHelper
{
public:
  RulerHelper();

  void Update(ScreenBase const & screen);
  bool IsVisible(ScreenBase const & screen) const;

  float GetRulerHalfHeight() const;
  float GetRulerPixelLength() const;
  int GetVerticalTextOffset() const;
  bool IsTextDirty() const;
  string const & GetRulerText() const;
  void GetTextInitInfo(string & alphabet, size_t & size) const;

private:
  double CalcMetresDiff(double value);

private:
  float m_pixelLength;
  int m_rangeIndex;
  string m_rulerText;
  mutable bool m_isTextDirty;
};

}
