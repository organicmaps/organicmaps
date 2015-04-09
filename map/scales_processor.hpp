#pragma once
#include "geometry/rect2d.hpp"


class ScreenBase;

class ScalesProcessor
{
  int m_tileSize;
  double m_visualScale;

public:
  ScalesProcessor();
  explicit ScalesProcessor(int tileSize);

  void SetParams(double visualScale, int tileSize);

  static m2::RectD const & GetWorldRect();
  inline int GetTileSize() const { return m_tileSize; }
  inline double GetVisualScale() const { return m_visualScale; }

  int GetTileScaleBase(ScreenBase const & s) const;
  int GetTileScaleBase(m2::RectD const & r) const;

  /// @return Adjusting base tile scale to look the same across devices with different
  /// tile size and visual scale values.
  int GetTileScaleIncrement() const;

  inline int GetDrawTileScale(int baseScale) const
  {
    return max(1, baseScale + GetTileScaleIncrement());
  }
  inline int GetDrawTileScale(ScreenBase const & s) const
  {
    return GetDrawTileScale(GetTileScaleBase(s));
  }
  inline int GetDrawTileScale(m2::RectD const & r) const
  {
    return GetDrawTileScale(GetTileScaleBase(r));
  }

  m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center) const;
  inline m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center) const
  {
    return GetRectForDrawScale(my::rounds(drawScale), center);
  }

  static int CalculateTileSize(int screenWidth, int screenHeight);

  double GetClipRectInflation() const;
};
