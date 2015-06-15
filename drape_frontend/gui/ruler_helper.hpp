#pragma once

#include "drape_frontend/animation/show_hide_animation.hpp"

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
  float GetMaxRulerPixelLength() const;
  int GetVerticalTextOffset() const;
  bool IsTextDirty() const;
  string const & GetRulerText() const;
  void ResetTextDirtyFlag();
  void GetTextInitInfo(string & alphabet, size_t & size) const;

private:
  double CalcMetresDiff(double value);
  void SetTextDirty();

private:
  float m_pixelLength;
  int m_rangeIndex;
  string m_rulerText;
  bool m_isTextDirty;
  mutable bool m_dirtyTextRequested;
  int m_currentDrawScale = 0;
};

}
