#pragma once

#include "common_structures.hpp"
#include "shape_view_params.hpp"
#include "intrusive_vector.hpp"

#include "../drape/pointers.hpp"
#include "../drape/texture_set_holder.hpp"
#include "../drape/overlay_handle.hpp"

#include "../geometry/spline.hpp"

#include "../base/string_utils.hpp"
#include "../base/buffer_vector.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace df
{

class TextLayout
{
  typedef dp::TextureSetHolder::GlyphRegion GlyphRegion;
public:
  TextLayout(strings::UniString const & string,
             df::FontDecl const & font,
             dp::RefPointer<dp::TextureSetHolder> textures);

  dp::OverlayHandle * LayoutText(FeatureID const & featureID,
                                 m2::PointF const & pivot,
                                 m2::PointF const & pixelOffset,
                                 float depth,
                                 vector<glsl_types::Quad4> & positions,
                                 vector<glsl_types::Quad4> & texCoord,
                                 vector<glsl_types::Quad4> & fontColor,
                                 vector<glsl_types::Quad4> & outlineColor) const;

  void InitPathText(float depth,
                    vector<glsl_types::Quad4> & texCoord,
                    vector<glsl_types::Quad4> & fontColor,
                    vector<glsl_types::Quad4> & outlineColor) const;
  void LayoutPathText(m2::Spline::iterator const & iterator,
                      float const scalePtoG,
                      IntrusiveVector<glsl_types::vec2> & positions,
                      bool isForwardDirection,
                      vector<m2::RectF> & rects,
                      const ScreenBase & screen) const;

  uint32_t GetGlyphCount() const;
  uint32_t GetTextureSet() const;
  float GetPixelLength() const;
  float GetPixelHeight() const;

private:
  void GetTextureQuad(GlyphRegion const & region, float depth, glsl_types::Quad4 & quad) const;
  float AccumulateAdvance(double const & currentValue, GlyphRegion const & reg2) const;
  void InitMetric(strings::UniChar const & unicodePoint, dp::RefPointer<dp::TextureSetHolder> textures);

public:
  void GetMetrics(int32_t const index, float & xOffset, float & yOffset, float & advance,
                  float & halfWidth, float & halfHeight) const;

private:
  buffer_vector<GlyphRegion, 32> m_metrics;
  df::FontDecl m_font;
  float m_textSizeRatio;
};

class SharedTextLayout
{
public:
  SharedTextLayout(TextLayout * layout);

  bool IsNull() const;
  void Reset(TextLayout * layout);

  TextLayout * operator->();
  TextLayout const * operator->() const;

private:
  shared_ptr<TextLayout> m_layout;
};

}
