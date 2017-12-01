#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "std/atomic.hpp"
#include "std/cstdint.hpp"
#include "std/noncopyable.hpp"

namespace df
{

class VisualParams : private noncopyable
{
public:
  static double const kMdpiScale;
  static double const kHdpiScale;
  static double const kXhdpiScale;
  static double const k6plusScale;
  static double const kXxhdpiScale;

  static void Init(double vs, uint32_t tileSize);
  static VisualParams & Instance();

  VisualParams();

  static string const & GetResourcePostfix(double visualScale);
  string const & GetResourcePostfix() const;

  double GetVisualScale() const;
  uint32_t GetTileSize() const;

  /// How many pixels around touch point are used to get bookmark or POI in consideration of visual scale.
  uint32_t GetTouchRectRadius() const;

  double GetDragThreshold() const;
  double GetScaleThreshold() const;

  struct GlyphVisualParams
  {
    float m_contrast;
    float m_gamma;
    float m_outlineContrast;
    float m_outlineGamma;
    float m_guiContrast;
    float m_guiGamma;
  };

  GlyphVisualParams const & GetGlyphVisualParams() const;
  uint32_t GetGlyphSdfScale() const;
  uint32_t GetGlyphBaseSize() const;
  double GetFontScale() const;
  void SetFontScale(double fontScale);

private:
  int m_tileSize;
  double m_visualScale;
  GlyphVisualParams m_glyphVisualParams;
  atomic<double> m_fontScale;
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

uint32_t CalculateTileSize(uint32_t screenWidth, uint32_t screenHeight);

double GetZoomLevel(double scale);
void ExtractZoomFactors(ScreenBase const & s, double & zoom, int & index, float & lerpCoef);
float InterpolateByZoomLevels(int index, float lerpCoef, std::vector<float> const & values);
m2::PointF InterpolateByZoomLevels(int index, float lerpCoef, std::vector<m2::PointF> const & values);
double GetNormalizedZoomLevel(double scale, int minZoom = 1);
double GetScale(double zoomLevel);

} // namespace df
