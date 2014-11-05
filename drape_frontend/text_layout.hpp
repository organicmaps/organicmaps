#pragma once

#include "shape_view_params.hpp"
#include "intrusive_vector.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/pointers.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../geometry/spline.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/string_utils.hpp"
#include "../base/buffer_vector.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace dp
{
  class OverlayHandle;
}

namespace df
{

class TextLayout
{
  typedef dp::TextureSetHolder::GlyphRegion GlyphRegion;
public:
  TextLayout(strings::UniString const & string,
             df::FontDecl const & font,
             dp::RefPointer<dp::TextureSetHolder> textures);

  void InitPathText(float depth,
                    vector<glsl::Quad4> & texCoord,
                    vector<glsl::Quad4> & fontColor,
                    vector<glsl::Quad1> & index,
                    dp::RefPointer<dp::TextureSetHolder> textures) const;
  void LayoutPathText(m2::Spline::iterator const & iterator,
                      float const scalePtoG,
                      IntrusiveVector<glsl::vec2> & positions,
                      bool isForwardDirection,
                      vector<m2::RectF> & rects,
                      ScreenBase const & screen) const;

  uint32_t GetGlyphCount() const;
  uint32_t GetTextureSet() const;
  float GetPixelLength() const;
  float GetPixelHeight() const;

private:
  float AccumulateAdvance(double const & currentValue, GlyphRegion const & reg2) const;
  void InitMetric(strings::UniChar const & unicodePoint, dp::RefPointer<dp::TextureSetHolder> textures);
  void GetTextureQuad(GlyphRegion const & region, float depth, glsl::Quad4 & quad) const;
  void GetMetrics(int32_t const index, float & xOffset, float & yOffset, float & advance,
                  float & halfWidth, float & halfHeight) const;

private:
  buffer_vector<GlyphRegion, 32> m_metrics;
  df::FontDecl m_font;
  float m_textSizeRatio;

  friend dp::OverlayHandle * LayoutText(FeatureID const & featureID,
                                        glsl::vec2 const & pivot,
                                        vector<TextLayout>::iterator & layoutIter,
                                        vector<glsl::vec2>::iterator & pixelOffsetIter,
                                        float depth,
                                        vector<glsl::Quad4> & positions,
                                        vector<glsl::Quad4> & texCoord,
                                        vector<glsl::Quad4> & color,
                                        vector<glsl::Quad1> & index,
                                        dp::RefPointer<dp::TextureSetHolder> textures,
                                        int count);
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
