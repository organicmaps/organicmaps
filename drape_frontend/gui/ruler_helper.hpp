#pragma once

#include "drape_frontend/animation/show_hide_animation.hpp"

#include <string>

class ScreenBase;

namespace gui
{
class RulerHelper
{
public:
  RulerHelper();

  void Update(ScreenBase const & screen);
  bool IsVisible(ScreenBase const & screen) const;
  void Invalidate();

  float GetRulerHalfHeight() const;
  float GetRulerPixelLength() const;
  float GetMaxRulerPixelLength() const;
  int GetVerticalTextOffset() const;
  bool IsTextDirty() const;
  std::string const & GetRulerText() const;
  void ResetTextDirtyFlag();
  void GetTextInitInfo(std::string & alphabet, uint32_t & size) const;

private:
  double CalcMetersDiff(double value);
  void SetTextDirty();

private:
  float m_pixelLength;
  int m_rangeIndex;
  std::string m_rulerText;
  bool m_isTextDirty;
  mutable bool m_dirtyTextRequested;
  int m_currentDrawScale = 0;
};
}  // namespace gui
