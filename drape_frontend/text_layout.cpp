#include "text_layout.hpp"

#include "../std/numeric.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

using glsl_types::vec4;
using glsl_types::Quad4;

namespace
{
  void FillColor(vector<Quad4> & data, dp::Color const & color)
  {
    Quad4 c;
    c.v[0] = c.v[1] = c.v[2] = c.v[3] = vec4(dp::ColorF(color));
    fill(data.begin(), data.end(), c);
  }
}

namespace df
{

class StraightTextHandle : public dp::OverlayHandle
{
public:
  StraightTextHandle(FeatureID const & id, m2::PointD const & pivot,
                     m2::PointD const & pxSize, m2::PointD const & offset,
                     double priority)
    : OverlayHandle(id, dp::LeftBottom, priority)
    , m_pivot(pivot)
    , m_offset(offset)
    , m_size(pxSize)
  {
  }

  m2::RectD GetPixelRect(ScreenBase const & screen) const
  {
    m2::PointD const pivot = screen.GtoP(m_pivot) + m_offset;
    return m2::RectD(pivot, pivot + m_size);
  }

private:
  m2::PointD m_pivot;
  m2::PointD m_offset;
  m2::PointD m_size;
};

namespace
{

#ifdef DEBUG
  void ValidateTextureSet(buffer_vector<dp::TextureSetHolder::GlyphRegion, 32> const & metrics)
  {
    if (metrics.size() < 2)
      return;

    ASSERT(metrics[0].IsValid(), ());
    uint32_t textureSet = metrics[0].GetTextureNode().m_textureSet;
    for (size_t i = 1; i < metrics.size(); ++i)
    {
      ASSERT(metrics[i].IsValid(), ());
      ASSERT_EQUAL(metrics[i].GetTextureNode().m_textureSet, textureSet, ());
    }
  }
#endif

  float const BASE_HEIGHT = 28.0f;
}

TextLayout::TextLayout(strings::UniString const & string,
                       df::FontDecl const & font,
                       dp::RefPointer<dp::TextureSetHolder> textures)
  : m_font(font)
  , m_textSizeRatio(font.m_size / BASE_HEIGHT)
{
  ASSERT(!string.empty(), ());
  m_metrics.reserve(string.size());
  for_each(string.begin(), string.end(), bind(&TextLayout::InitMetric, this, _1, textures));
#ifdef DEBUG
  ValidateTextureSet(m_metrics);
#endif
}

dp::OverlayHandle * TextLayout::LayoutText(const FeatureID & featureID,
                                           m2::PointF const & pivot,
                                           m2::PointF const & pixelOffset,
                                           float depth,
                                           vector<Quad4> & positions,
                                           vector<Quad4> & texCoord,
                                           vector<Quad4> & fontColor,
                                           vector<Quad4> & outlineColor) const
{
  STATIC_ASSERT(sizeof(vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(Quad4) == 4 * sizeof(vec4));

  size_t glyphCount = GetGlyphCount();
  ASSERT(glyphCount <= positions.size(), ());
  ASSERT(glyphCount <= texCoord.size(), ());
  ASSERT(glyphCount <= fontColor.size(), ());
  ASSERT(glyphCount <= outlineColor.size(), ());

  FillColor(fontColor, m_font.m_color);
  FillColor(outlineColor, m_font.m_outlineColor);

  float glyphOffset = 0.0;
  for (size_t i = 0; i < glyphCount; ++i)
  {
    GlyphRegion const & region = m_metrics[i];
    ASSERT(region.IsValid(), ());
    GetTextureQuad(region, depth, texCoord[i]);

    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);

    xOffset *= m_textSizeRatio;
    yOffset *= m_textSizeRatio;
    advance *= m_textSizeRatio;

    m2::PointU size;
    region.GetPixelSize(size);
    double const h = size.y * m_textSizeRatio;
    double const w = size.x * m_textSizeRatio;

    Quad4 & position = positions[i];
    position.v[0] = vec4(pivot, m2::PointF(glyphOffset + xOffset, yOffset) + pixelOffset);
    position.v[1] = vec4(pivot, m2::PointF(glyphOffset + xOffset, yOffset + h) + pixelOffset);
    position.v[2] = vec4(pivot, m2::PointF(glyphOffset + w + xOffset, yOffset) + pixelOffset);
    position.v[3] = vec4(pivot, m2::PointF(glyphOffset + w + xOffset, yOffset + h) + pixelOffset);
    glyphOffset += advance;
  }

  return new StraightTextHandle(featureID, pivot, m2::PointD(glyphOffset, m_font.m_size),
                                pixelOffset, depth);
}

void TextLayout::InitPathText(float depth,
                              vector<glsl_types::Quad4> & texCoord,
                              vector<glsl_types::Quad4> & fontColor,
                              vector<glsl_types::Quad4> & outlineColor) const
{
  STATIC_ASSERT(sizeof(vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(Quad4) == 4 * sizeof(vec4));

  size_t glyphCount = GetGlyphCount();
  ASSERT(glyphCount <= texCoord.size(), ());
  ASSERT(glyphCount <= fontColor.size(), ());
  ASSERT(glyphCount <= outlineColor.size(), ());

  FillColor(fontColor, m_font.m_color);
  FillColor(outlineColor, m_font.m_outlineColor);

  for (size_t i = 0; i < glyphCount; ++i)
    GetTextureQuad(m_metrics[i], depth, texCoord[i]);
}

void TextLayout::LayoutPathText(m2::Spline::iterator const & iterator,
                                float const scalePtoG,
                                IntrusiveVector<glsl_types::vec2> & positions,
                                bool isForwardDirection) const
{
  if (!isForwardDirection)
    positions.SetFillDirection(df::Backward);

  m2::Spline::iterator itr = iterator;

  uint32_t glyphCount = GetGlyphCount();
  int32_t startIndex = isForwardDirection ? 0 : glyphCount - 1;
  int32_t endIndex = isForwardDirection ? glyphCount : -1;
  int32_t incSign = isForwardDirection ? 1 : -1;

  for (int32_t i = startIndex; i != endIndex; i += incSign)
  {
    float xOffset, yOffset, advance;
    float halfWidth, halfHeight;
    GetMetrics(i, xOffset, yOffset, advance, halfWidth, halfHeight);
    advance *= scalePtoG;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    m2::PointF const pos = itr.m_pos;
    itr.Step(advance);
    ASSERT(!itr.BeginAgain(), ());

    m2::PointF dir = itr.m_avrDir.Normalize();
    m2::PointF norm(-dir.y, dir.x);
    m2::PointF norm2 = norm;
    dir *= halfWidth * scalePtoG;
    norm *= halfHeight * scalePtoG;

    float const halfFontSize = m_textSizeRatio * scalePtoG / 2.0f;
    m2::PointF const dirComponent = dir * xOffset / halfWidth;
    m2::PointF const normalComponent = -norm * incSign * yOffset / halfHeight;
    m2::PointF const fontSizeComponent = norm2 * incSign * halfFontSize;
    m2::PointF const pivot = dirComponent + normalComponent + pos - fontSizeComponent;

    positions.PushBack(glsl_types::vec2(pivot - dir + norm));
    positions.PushBack(glsl_types::vec2(pivot - dir - norm));
    positions.PushBack(glsl_types::vec2(pivot + dir + norm));
    positions.PushBack(glsl_types::vec2(pivot + dir - norm));
  }
}

uint32_t TextLayout::GetGlyphCount() const
{
  return m_metrics.size();
}

uint32_t TextLayout::GetTextureSet() const
{
  return m_metrics[0].GetTextureNode().m_textureSet;
}

float TextLayout::GetPixelLength() const
{
  return m_textSizeRatio * accumulate(m_metrics.begin(), m_metrics.end(), 0.0,
                                      bind(&TextLayout::AccumulateAdvance, this, _1, _2));
}

float TextLayout::GetPixelHeight() const
{
  return m_font.m_size;
}

void TextLayout::GetTextureQuad(GlyphRegion const & region,
                                float depth,
                                Quad4 & quad) const
{
  ASSERT(region.IsValid(), ());

  m2::RectF const & rect = region.GetTexRect();
  uint8_t needOutline = m_font.m_needOutline ? 1 : 0;
  float textureOffset = static_cast<float>((region.GetTextureNode().m_textureOffset << 1) + needOutline);
  quad.v[0] = vec4(rect.minX(), rect.minY(), textureOffset, depth);
  quad.v[1] = vec4(rect.minX(), rect.maxY(), textureOffset, depth);
  quad.v[2] = vec4(rect.maxX(), rect.minY(), textureOffset, depth);
  quad.v[3] = vec4(rect.maxX(), rect.maxY(), textureOffset, depth);
}

float TextLayout::AccumulateAdvance(double const & currentValue, GlyphRegion const & reg1) const
{
  ASSERT(reg1.IsValid(), ());

  return currentValue + reg1.GetAdvance();
}

void TextLayout::InitMetric(strings::UniChar const & unicodePoint,
                            dp::RefPointer<dp::TextureSetHolder> textures)
{
  GlyphRegion region;
  if (textures->GetGlyphRegion(unicodePoint, region))
    m_metrics.push_back(region);
}

void TextLayout::GetMetrics(int32_t const index, float & xOffset, float & yOffset, float & advance,
                            float & halfWidth, float & halfHeight) const
{
  GlyphRegion const & region = m_metrics[index];
  m2::PointU size;
  region.GetPixelSize(size);
  region.GetMetrics(xOffset, yOffset, advance);

  halfWidth = m_textSizeRatio * size.x / 2.0f;
  halfHeight = m_textSizeRatio * size.y / 2.0f;

  xOffset = xOffset * m_textSizeRatio + halfWidth;
  yOffset = yOffset * m_textSizeRatio + halfHeight;
  advance *= m_textSizeRatio;
}

///////////////////////////////////////////////////////////////
SharedTextLayout::SharedTextLayout(TextLayout * layout)
  : m_layout(layout)
{
}

bool SharedTextLayout::IsNull() const
{
  return m_layout == NULL;
}

void SharedTextLayout::Reset(TextLayout * layout)
{
  m_layout.reset(layout);
}

TextLayout * SharedTextLayout::operator->()
{
  return m_layout.get();
}

TextLayout const * SharedTextLayout::operator->() const
{
  return m_layout.get();
}



}
