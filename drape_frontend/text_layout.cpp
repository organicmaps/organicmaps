#include "text_layout.hpp"

#include "../std/numeric.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/algorithm.hpp"
#include "../std/limits.hpp"

using glsl_types::vec4;
using glsl_types::Quad1;
using glsl_types::Quad4;

namespace
{
void FillColor(vector<Quad4> & colors,
               dp::TextureSetHolder::ColorRegion & region,
               dp::Color const & base, dp::Color const & outline,
               dp::RefPointer<dp::TextureSetHolder> textures)
  {
  dp::ColorKey key(base.GetColorInInt());
  textures->GetColorRegion(key, region);
  m2::RectF const & rect = region.GetTexRect();
  key.SetColor(outline.GetColorInInt());
  textures->GetColorRegion(key, region);
  m2::RectF const & outlineRect = region.GetTexRect();

  vec4 clrs(rect.RightTop(), outlineRect.RightTop());
  Quad4 f(clrs, clrs, clrs, clrs);
  fill(colors.begin(), colors.end(), f);
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

  void GetPixelShape(ScreenBase const & screen, Rects & rects) const
  {
    m2::RectD rd = GetPixelRect(screen);
    rects.push_back(m2::RectF(rd.minX(), rd.minY(), rd.maxX(), rd.maxY()));
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

dp::OverlayHandle * LayoutText(const FeatureID & featureID,
                               m2::PointF const & pivot,
                               vector<TextLayout>::iterator & layoutIter,
                               vector<m2::PointF>::iterator & pixelOffsetIter,
                               float depth,
                               vector<glsl_types::Quad4> & positions,
                               vector<glsl_types::Quad4> & texCoord,
                               vector<glsl_types::Quad4> & color,
                               vector<glsl_types::Quad1> & index,
                               dp::RefPointer<dp::TextureSetHolder> textures,
                               int count)
{
  STATIC_ASSERT(sizeof(vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(Quad4) == 4 * sizeof(vec4));

  dp::TextureSetHolder::ColorRegion region;
  FillColor(color, region, layoutIter->m_font.m_color, layoutIter->m_font.m_outlineColor, textures);
  float texIndex = static_cast<float>(region.GetTextureNode().m_textureOffset);
  Quad1 f(texIndex, texIndex, texIndex, texIndex);
  fill(index.begin(), index.end(), f);

  int counter = 0;
  m2::PointD size(0.0, 0.0);
  m2::PointF offset(numeric_limits<float>::max(), numeric_limits<float>::max());
  float maxOffset = numeric_limits<float>::min();
  for (int j = 0; j < count; ++j)
  {
    float glyphOffset = 0.0;
    float textRatio = layoutIter->m_textSizeRatio;
    float maxHeight = 0.0f;
    float minHeight = numeric_limits<float>::max();
    uint32_t glyphCount = layoutIter->GetGlyphCount();
    double w;
    for (size_t i = 0; i < glyphCount; ++i)
    {
      dp::TextureSetHolder::GlyphRegion const & region = layoutIter->m_metrics[i];
      ASSERT(region.IsValid(), ());
      layoutIter->GetTextureQuad(region, depth, texCoord[counter]);

      float xOffset, yOffset, advance;
      region.GetMetrics(xOffset, yOffset, advance);

      xOffset *= textRatio;
      yOffset *= textRatio;
      advance *= textRatio;

      m2::PointU size;
      region.GetPixelSize(size);
      double const h = size.y * textRatio;
      w = size.x * textRatio;
      maxHeight = max((float)h, maxHeight);
      minHeight = min(yOffset, minHeight);

      Quad4 & position = positions[counter++];
      position.v[0] = vec4(pivot, m2::PointF(glyphOffset + xOffset, yOffset) + *pixelOffsetIter);
      position.v[1] = vec4(pivot, m2::PointF(glyphOffset + xOffset, yOffset + h) + *pixelOffsetIter);
      position.v[2] = vec4(pivot, m2::PointF(glyphOffset + w + xOffset, yOffset) + *pixelOffsetIter);
      position.v[3] = vec4(pivot, m2::PointF(glyphOffset + w + xOffset, yOffset + h) + *pixelOffsetIter);
      glyphOffset += advance;
    }
    glyphOffset += w / 2.0f;
    size.x = max(size.x, (double)glyphOffset);
    offset.x = min(offset.x, pixelOffsetIter->x);
    offset.y = min(offset.y, pixelOffsetIter->y + minHeight);
    maxOffset = max(maxOffset, pixelOffsetIter->y + minHeight);
    size.y = max(size.y, (double)maxHeight);
    ++layoutIter;
    ++pixelOffsetIter;
  }
  size.y += maxOffset - offset.y;
  return new StraightTextHandle(featureID, pivot, size, offset, depth);
}

void TextLayout::InitPathText(float depth,
                              vector<glsl_types::Quad4> & texCoord,
                              vector<glsl_types::Quad4> & fontColor,
                              vector<glsl_types::Quad1> & index,
                              dp::RefPointer<dp::TextureSetHolder> textures) const
{
  STATIC_ASSERT(sizeof(vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(Quad4) == 4 * sizeof(vec4));

  size_t glyphCount = GetGlyphCount();
  ASSERT(glyphCount <= texCoord.size(), ());
  ASSERT(glyphCount <= fontColor.size(), ());
  ASSERT(glyphCount <= index.size(), ());

  dp::TextureSetHolder::ColorRegion region;
  FillColor(fontColor, region, m_font.m_color, m_font.m_outlineColor, textures);
  float texIndex = static_cast<float>(region.GetTextureNode().m_textureOffset);
  Quad1 f(texIndex, texIndex, texIndex, texIndex);
  fill(index.begin(), index.end(), f);

  for (size_t i = 0; i < glyphCount; ++i)
    GetTextureQuad(m_metrics[i], depth, texCoord[i]);
}

void TextLayout::LayoutPathText(m2::Spline::iterator const & iterator,
                                float const scalePtoG,
                                IntrusiveVector<glsl_types::vec2> & positions,
                                bool isForwardDirection,
                                vector<m2::RectF> & rects,
                                ScreenBase const & screen) const
{
  if (!isForwardDirection)
    positions.SetFillDirection(df::Backward);

  m2::Spline::iterator itr = iterator;

  uint32_t glyphCount = GetGlyphCount();
  int32_t startIndex = isForwardDirection ? 0 : glyphCount - 1;
  int32_t endIndex = isForwardDirection ? glyphCount : -1;
  int32_t incSign = isForwardDirection ? 1 : -1;

  m2::PointF accum = itr.m_dir;
  float const koef = 0.7f;

  rects.resize(GetGlyphCount());

  for (int32_t i = startIndex; i != endIndex; i += incSign)
  {
    float xOffset, yOffset, advance;
    float halfWidth, halfHeight;
    GetMetrics(i, xOffset, yOffset, advance, halfWidth, halfHeight);
    advance *= scalePtoG;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    m2::PointF pos = itr.m_pos;
    itr.Step(advance);
    ASSERT(!itr.BeginAgain(), ());

    m2::PointF dir = (itr.m_avrDir.Normalize() * koef + accum * (1.0f - koef)).Normalize();
    accum = dir;
    m2::PointF norm(-dir.y, dir.x);
    dir *= halfWidth * scalePtoG;
    norm *= halfHeight * scalePtoG;

    m2::PointF const dirComponent = dir * xOffset / halfWidth;
    m2::PointF const normalComponent = -norm * incSign * yOffset / halfHeight;
    m2::PointF const pivot = dirComponent + normalComponent + pos;

    positions.PushBack(glsl_types::vec2(pivot - dir + norm));
    positions.PushBack(glsl_types::vec2(pivot - dir - norm));
    positions.PushBack(glsl_types::vec2(pivot + dir + norm));
    positions.PushBack(glsl_types::vec2(pivot + dir - norm));

    pos = screen.GtoP(pivot);
    float maxDim = max(halfWidth, halfHeight);
    m2::PointF const maxDir(maxDim, maxDim);
    rects[i] = m2::RectF(pos - maxDir, pos + maxDir);
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
