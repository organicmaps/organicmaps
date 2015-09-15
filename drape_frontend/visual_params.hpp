#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "std/cstdint.hpp"

#include "std/noncopyable.hpp"

namespace df
{

extern uint32_t const YotaDevice;

class VisualParams : private noncopyable
{
public:
  static void Init(double vs, uint32_t tileSize, vector<uint32_t> const & additionalOptions = vector<uint32_t>());
  static VisualParams & Instance();

  VisualParams();

  static string const & GetResourcePostfix(double visualScale, bool isYotaDevice = false);
  string const & GetResourcePostfix() const;

  double GetVisualScale() const;
  uint32_t GetTileSize() const;

  /// How many pixels around touch point are used to get bookmark or POI in consideration of visual scale
  uint32_t GetTouchRectRadius() const;
  double GetDragThreshold() const;

private:
  int m_tileSize;
  double m_visualScale;
  bool m_isYotaDevice;
};

m2::RectD const & GetWorldRect();

int GetTileScaleBase(ScreenBase const & s, uint32_t tileSize);
int GetTileScaleBase(ScreenBase const & s);
int GetTileScaleBase(m2::RectD const & r);

/// @return Adjusting base tile scale to look the same across devices with different
/// tile size and visual scale values.
int GetTileScaleIncrement(uint32_t tileSize, double visualScale);
int GetTileScaleIncrement();

int GetDrawTileScale(int baseScale, uint32_t tileSize, double visualScale);
int GetDrawTileScale(ScreenBase const & s, uint32_t tileSize, double visualScale);
int GetDrawTileScale(m2::RectD const & r, uint32_t tileSize, double visualScale);
int GetDrawTileScale(int baseScale);
int GetDrawTileScale(ScreenBase const & s);
int GetDrawTileScale(m2::RectD const & r);

m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale);
m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale);
m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center);
m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center);

int CalculateTileSize(int screenWidth, int screenHeight);

} // namespace df
