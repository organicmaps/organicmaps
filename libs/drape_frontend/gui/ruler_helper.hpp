#pragma once

#include <cstdint>
#include <string>

class ScreenBase;

namespace gui
{
class RulerHelper
{
public:
  RulerHelper();

  void Update(ScreenBase const & screen);
  static bool IsVisible(ScreenBase const & screen);
  void Invalidate();

  static float GetRulerHalfHeight();
  float GetRulerPixelLength() const { return m_pixelLength; }
  static int GetVerticalTextOffset();
  bool IsTextDirty() const;
  std::string const & GetRulerText() const;
  void ResetTextDirtyFlag();
  static void GetTextInitInfo(std::string & alphabet, uint32_t & size);

private:
  double CalcMetersDiff(double value);
  void SetTextDirty();

private:
  std::string m_rulerText;
  float m_pixelLength;
  int m_rangeIndex;

  int m_currentDrawScale = 0;
  bool m_isTextDirty;
  mutable bool m_dirtyTextRequested;
};
}  // namespace gui
