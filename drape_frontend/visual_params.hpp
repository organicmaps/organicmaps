#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/macros.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace df
{
double constexpr kBoundingBoxScale = 1.2;

class VisualParams
{
public:
  static double const kMdpiScale;
  static double const kHdpiScale;
  static double const kXhdpiScale;
  static double const k6plusScale;
  static double const kXxhdpiScale;
  static double const kXxxhdpiScale;

  static void Init(double vs, uint32_t tileSize);
  static VisualParams & Instance();

  static std::string const & GetResourcePostfix(double visualScale);
  std::string const & GetResourcePostfix() const;

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
  bool IsSdfPrefered() const;
  uint32_t GetGlyphSdfScale() const;
  uint32_t GetGlyphBaseSize() const;
  double GetFontScale() const;
  void SetFontScale(double fontScale);

  // This method can be called ONLY if rendering is disabled.
  void SetVisualScale(double visualScale);

private:
  VisualParams();

  uint32_t m_tileSize;
  double m_visualScale;
  GlyphVisualParams m_glyphVisualParams;
  std::atomic<double> m_fontScale;

  DISALLOW_COPY_AND_MOVE(VisualParams);
};

m2::RectD const & GetWorldRect();

int GetTileScaleBase(ScreenBase const & s, uint32_t tileSize);
int GetTileScaleBase(ScreenBase const & s);
int GetTileScaleBase(m2::RectD const & r);
double GetTileScaleBase(double drawScale);

// @return Adjusting base tile scale to look the same across devices with different
// tile size and visual scale values.
int GetTileScaleIncrement(uint32_t tileSize, double visualScale);
int GetTileScaleIncrement();

int GetDrawTileScale(int baseScale, uint32_t tileSize, double visualScale);
int GetDrawTileScale(ScreenBase const & s, uint32_t tileSize, double visualScale);
int GetDrawTileScale(m2::RectD const & r, uint32_t tileSize, double visualScale);
int GetDrawTileScale(int baseScale);
double GetDrawTileScale(double baseScale);
int GetDrawTileScale(ScreenBase const & s);
int GetDrawTileScale(m2::RectD const & r);

m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale);
m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale);
m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center);
m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center);

uint32_t CalculateTileSize(uint32_t screenWidth, uint32_t screenHeight);

void ExtractZoomFactors(ScreenBase const & s, double & zoom, int & index, float & lerpCoef);
float InterpolateByZoomLevels(int index, float lerpCoef, std::vector<float> const & values);
m2::PointF InterpolateByZoomLevels(int index, float lerpCoef, std::vector<m2::PointF> const & values);
double GetNormalizedZoomLevel(double screenScale, int minZoom = 1);
double GetScreenScale(double zoomLevel);
double GetZoomLevel(double screenScale);
}  // namespace df
