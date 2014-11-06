#include "text_layout.hpp"

#include "../drape/glsl_func.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/numeric.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/limits.hpp"

namespace
{

void FillColor(vector<glsl::Quad4> & colors,
               dp::TextureSetHolder::ColorRegion & region,
               dp::Color const & base, dp::Color const & outline,
               dp::RefPointer<dp::TextureSetHolder> textures)
{
  dp::ColorKey key(base.GetColorInInt());
  textures->GetColorRegion(key, region);
  m2::PointF const color = region.GetTexRect().Center();
  key.SetColor(outline.GetColorInInt());
  textures->GetColorRegion(key, region);
  m2::PointF const mask = region.GetTexRect().Center();

  glsl::vec4 clrs(color.x, color.y, mask.x, mask.y);
  fill(colors.begin(), colors.end(), glsl::Quad4(clrs, clrs, clrs, clrs));
}

}

namespace df
{

class StraightTextHandle : public dp::OverlayHandle
{
public:
  StraightTextHandle(FeatureID const & id, glsl::vec2 const & pivot,
                     glsl::vec2 const & pxSize, glsl::vec2 const & offset,
                     double priority)
    : OverlayHandle(id, dp::LeftBottom, priority)
    , m_pivot(pivot)
    , m_offset(offset)
    , m_size(pxSize)
  {
  }

  m2::RectD GetPixelRect(ScreenBase const & screen) const
  {
    m2::PointD const pivot = screen.GtoP(m2::PointD(m_pivot.x, m_pivot.y)) + m2::PointD(m_offset.x, m_offset.y);
    return m2::RectD(pivot, pivot + glsl::ToPoint(m_size));
  }

  void GetPixelShape(ScreenBase const & screen, Rects & rects) const
  {
    m2::RectD rd = GetPixelRect(screen);
    rects.push_back(m2::RectF(rd.minX(), rd.minY(), rd.maxX(), rd.maxY()));
  }
private:
  glsl::vec2 m_pivot;
  glsl::vec2 m_offset;
  glsl::vec2 m_size;
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

dp::OverlayHandle * LayoutText(FeatureID const & featureID,
                               glsl::vec2 const & pivot,
                               vector<TextLayout>::iterator & layoutIter,
                               vector<glsl::vec2>::iterator & pixelOffsetIter,
                               float depth,
                               vector<glsl::Quad4> & positions,
                               vector<glsl::Quad4> & texCoord,
                               vector<glsl::Quad4> & color,
                               vector<glsl::Quad1> & index,
                               dp::RefPointer<dp::TextureSetHolder> textures,
                               int count)
{
  STATIC_ASSERT(sizeof(glsl::vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(glsl::Quad4) == 4 * sizeof(glsl::vec4));

  dp::TextureSetHolder::ColorRegion region;
  FillColor(color, region, layoutIter->m_font.m_color, layoutIter->m_font.m_outlineColor, textures);
  float texIndex = region.GetTextureNode().GetOffset();
  glsl::Quad1 f(texIndex, texIndex, texIndex, texIndex);
  fill(index.begin(), index.end(), f);

  int counter = 0;
  glsl::vec2 size(0.0, 0.0);
  glsl::vec2 offset(numeric_limits<float>::max(), numeric_limits<float>::max());
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

      glsl::Quad4 & position = positions[counter++];
      position[0] = glsl::vec4(pivot, glsl::vec2(glyphOffset + xOffset, yOffset) + *pixelOffsetIter);
      position[1] = glsl::vec4(pivot, glsl::vec2(glyphOffset + xOffset, yOffset + h) + *pixelOffsetIter);
      position[2] = glsl::vec4(pivot, glsl::vec2(glyphOffset + w + xOffset, yOffset) + *pixelOffsetIter);
      position[3] = glsl::vec4(pivot, glsl::vec2(glyphOffset + w + xOffset, yOffset + h) + *pixelOffsetIter);
      glyphOffset += advance;
    }
    glyphOffset += w / 2.0f;
    size.x = max(size.x, glyphOffset);
    offset.x = min(offset.x, pixelOffsetIter->x);
    offset.y = min(offset.y, pixelOffsetIter->y + minHeight);
    maxOffset = max(maxOffset, pixelOffsetIter->y + minHeight);
    size.y = max(size.y, maxHeight);
    ++layoutIter;
    ++pixelOffsetIter;
  }
  size.y += maxOffset - offset.y;
  return new StraightTextHandle(featureID, pivot, size, offset, depth);
}

void TextLayout::InitPathText(float depth,
                              vector<glsl::Quad4> & texCoord,
                              vector<glsl::Quad4> & fontColor,
                              vector<glsl::Quad1> & index,
                              dp::RefPointer<dp::TextureSetHolder> textures) const
{
  STATIC_ASSERT(sizeof(glsl::vec4) == 4 * sizeof(float));
  STATIC_ASSERT(sizeof(glsl::Quad4) == 4 * sizeof(glsl::vec4));

  size_t glyphCount = GetGlyphCount();
  ASSERT(glyphCount <= texCoord.size(), ());
  ASSERT(glyphCount <= fontColor.size(), ());
  ASSERT(glyphCount <= index.size(), ());

  dp::TextureSetHolder::ColorRegion region;
  FillColor(fontColor, region, m_font.m_color, m_font.m_outlineColor, textures);
  float texIndex = region.GetTextureNode().GetOffset();
  glsl::Quad1 f(texIndex, texIndex, texIndex, texIndex);
  fill(index.begin(), index.end(), f);

  for (size_t i = 0; i < glyphCount; ++i)
    GetTextureQuad(m_metrics[i], depth, texCoord[i]);
}

void TextLayout::LayoutPathText(m2::Spline::iterator const & iterator,
                                float const scalePtoG,
                                IntrusiveVector<glsl::vec2> & positions,
                                bool isForwardDirection,
                                vector<m2::RectF> & rects,
                                ScreenBase const & screen) const
{
  if (!isForwardDirection)
    positions.SetFillDirection(df::Backward);

  m2::Spline::iterator itr = iterator;
  m2::Spline::iterator leftTentacle, rightTentacle;

  uint32_t glyphCount = GetGlyphCount();
  int32_t startIndex = isForwardDirection ? 0 : glyphCount - 1;
  int32_t endIndex = isForwardDirection ? glyphCount : -1;
  int32_t incSign = isForwardDirection ? 1 : -1;

  float const tentacleLength = m_font.m_size * 1.6f * scalePtoG;

  rects.resize(GetGlyphCount());

  for (int32_t i = startIndex; i != endIndex; i += incSign)
  {
    float xOffset, yOffset, advance;
    float halfWidth, halfHeight;
    GetMetrics(i, xOffset, yOffset, advance, halfWidth, halfHeight);
    advance *= scalePtoG;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    glsl::vec2 pos = glsl::ToVec2(itr.m_pos);
    double cosa = 1.0;

    glsl::vec2 dir;
    glsl::vec2 posOffset;
    if (itr.GetLength() <= tentacleLength || itr.GetLength() >= itr.GetFullLength() - tentacleLength)
    {
      dir = glsl::normalize(glsl::ToVec2(itr.m_avrDir));
    }
    else
    {
      leftTentacle = itr;
      leftTentacle.StepBack(tentacleLength);
      rightTentacle = itr;
      rightTentacle.Step(tentacleLength);

      glsl::vec2 const leftAvrDir = glsl::normalize(glsl::ToVec2(leftTentacle.m_avrDir));
      glsl::vec2 const rightAvrDir = glsl::normalize(glsl::ToVec2(rightTentacle.m_avrDir));
      dir = glsl::normalize(rightAvrDir - leftAvrDir);
      cosa = glsl::dot(-leftAvrDir, leftAvrDir);

      glsl::vec2 const leftPos(glsl::ToVec2(leftTentacle.m_pos));
      glsl::vec2 const rightPos(glsl::ToVec2(rightTentacle.m_pos));
      posOffset = (leftPos + rightPos) / 2.0f;
      pos = (posOffset + glsl::ToVec2(itr.m_pos)) / 2.0f;
    }

    itr.Step(advance + (1.0 - cosa) * advance * 0.1);
    //ASSERT(!itr.BeginAgain(), ());

    glsl::vec2 norm(-dir.y, dir.x);
    dir *= halfWidth * scalePtoG;
    norm *= halfHeight * scalePtoG;

    glsl::vec2 dirComponent;
    if (isForwardDirection)
      dirComponent = dir * xOffset / halfWidth;
    else
      dirComponent = dir * (2.0f * halfWidth - xOffset) / halfWidth;

    glsl::vec2 const normalComponent = -norm * (float)incSign * yOffset / halfHeight;
    glsl::vec2 const pivot = dirComponent + normalComponent + pos;

    positions.PushBack(glsl::vec2(pivot - dir + norm));
    positions.PushBack(glsl::vec2(pivot - dir - norm));
    positions.PushBack(glsl::vec2(pivot + dir + norm));
    positions.PushBack(glsl::vec2(pivot + dir - norm));

    m2::PointF resPos = screen.GtoP(glsl::ToPoint(pivot));
    float maxDim = max(halfWidth, halfHeight);
    m2::PointF const maxDir(maxDim, maxDim);
    rects[i] = m2::RectF(resPos - maxDir, resPos + maxDir);
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
                                glsl::Quad4 & quad) const
{
  ASSERT(region.IsValid(), ());

  m2::RectF const & rect = region.GetTexRect();
  uint8_t needOutline = m_font.m_needOutline ? 1 : 0;
  float textureOffset = static_cast<float>((region.GetTextureNode().m_textureOffset << 1) + needOutline);
  quad[0] = glsl::vec4(rect.minX(), rect.minY(), textureOffset, depth);
  quad[1] = glsl::vec4(rect.minX(), rect.maxY(), textureOffset, depth);
  quad[2] = glsl::vec4(rect.maxX(), rect.minY(), textureOffset, depth);
  quad[3] = glsl::vec4(rect.maxX(), rect.maxY(), textureOffset, depth);
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
