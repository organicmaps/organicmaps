#pragma once
#include "indexer/scales.hpp"

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
double constexpr kMaxVisualScale = 4.0;

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
  /// This is a scale factor to decrease extending of bbox for POI icons. It could be removed with new style
  double GetPoiExtendScale() const;
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
  double m_poiExtendScale;
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

template <class T> class ArrayView
{
  T const * m_arr;
  size_t m_size;

public:
  using value_type = T;

  template <class ArrayT> ArrayView(ArrayT const & arr) : m_arr(arr.data()), m_size(arr.size()) {}

  ArrayView(ArrayView const &) = default;
  ArrayView(ArrayView &&) = default;

  size_t size() const { return m_size; }
  T operator[](size_t i) const
  {
    ASSERT_LESS(i, m_size, ());
    return m_arr[i];
  }
};

template <class ArrayT> typename ArrayT::value_type
InterpolateByZoomLevels(int index, float lerpCoef, ArrayT const & values)
{
  ASSERT_GREATER_OR_EQUAL(index, 0, ());
  ASSERT_GREATER(values.size(), scales::UPPER_STYLE_SCALE, ());
  if (index < scales::UPPER_STYLE_SCALE)
    return values[index] + (values[index + 1] - values[index]) * lerpCoef;
  return values[scales::UPPER_STYLE_SCALE];
}

double GetNormalizedZoomLevel(double screenScale, int minZoom = 1);
double GetScreenScale(double zoomLevel);
double GetZoomLevel(double screenScale);

float CalculateRadius(ScreenBase const & screen, ArrayView<float> const & zoom2radius);
}  // namespace df
