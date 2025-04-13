#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/font_constants.hpp"

#include <algorithm>
#include <iterator>  // std::reverse_iterator
#include <numeric>

namespace df
{
namespace
{
float constexpr kValidSplineTurn = 0.96f;

class TextGeometryGenerator
{
public:
  TextGeometryGenerator(dp::TextureManager::ColorRegion const & color,
                        gpu::TTextStaticVertexBuffer & buffer)
    : m_colorCoord(glsl::ToVec2(color.GetTexRect().Center()))
    , m_buffer(buffer)
  {}

  void SetPenPosition(glsl::vec2 const & penOffset) {}

  void operator() (dp::TextureManager::GlyphRegion const & glyph, dp::text::GlyphMetrics const &)
  {
    m2::RectF const & mask = glyph.GetTexRect();

    m_buffer.emplace_back(m_colorCoord, glsl::ToVec2(mask.LeftTop()));
    m_buffer.emplace_back(m_colorCoord, glsl::ToVec2(mask.LeftBottom()));
    m_buffer.emplace_back(m_colorCoord, glsl::ToVec2(mask.RightTop()));
    m_buffer.emplace_back(m_colorCoord, glsl::ToVec2(mask.RightBottom()));
  }

protected:
  glsl::vec2 m_colorCoord;
  gpu::TTextStaticVertexBuffer & m_buffer;
};

class StraightTextGeometryGenerator
{
public:
  StraightTextGeometryGenerator(glsl::vec4 const & pivot,
                                glsl::vec2 const & pixelOffset, float textRatio,
                                gpu::TTextDynamicVertexBuffer & dynBuffer)
    : m_pivot(pivot)
    , m_pixelOffset(pixelOffset)
    , m_buffer(dynBuffer)
    , m_textRatio(textRatio)
  {}

  void SetPenPosition(glsl::vec2 const & penPosition)
  {
    m_penPosition = penPosition;
    m_isFirstGlyph = true;
  }

  void operator()(dp::TextureManager::GlyphRegion const & glyphRegion, dp::text::GlyphMetrics const & metrics)
  {
    if (!glyphRegion.IsValid())
      return;
    m2::PointF const pixelSize = glyphRegion.GetPixelSize() * m_textRatio;

    float const xOffset = metrics.m_xOffset * m_textRatio;
    float const yOffset = metrics.m_yOffset * m_textRatio;

    float const upVector = -static_cast<int32_t>(pixelSize.y) - yOffset;
    float const bottomVector = -yOffset;

    if (m_isFirstGlyph)
    {
      m_isFirstGlyph = false;
      m_penPosition.x -= (xOffset + dp::kSdfBorder * m_textRatio);
    }

    auto const pixelPlusPen = m_pixelOffset + m_penPosition;
    m_buffer.emplace_back(m_pivot, pixelPlusPen + glsl::vec2(xOffset, bottomVector));
    m_buffer.emplace_back(m_pivot, pixelPlusPen + glsl::vec2(xOffset, upVector));
    m_buffer.emplace_back(m_pivot, pixelPlusPen + glsl::vec2(pixelSize.x + xOffset, bottomVector));
    m_buffer.emplace_back(m_pivot, pixelPlusPen + glsl::vec2(pixelSize.x + xOffset, upVector));
    // TODO(AB): yAdvance is always zero for horizontal text layouts.
    m_penPosition += glsl::vec2(metrics.m_xAdvance * m_textRatio, metrics.m_yAdvance * m_textRatio);
  }

private:
  glsl::vec4 const & m_pivot;
  glsl::vec2 m_pixelOffset;
  glsl::vec2 m_penPosition;
  gpu::TTextDynamicVertexBuffer & m_buffer;
  float m_textRatio = 0.0f;
  bool m_isFirstGlyph = true;
};

class TextOutlinedGeometryGenerator
{
public:
  TextOutlinedGeometryGenerator(dp::TextureManager::ColorRegion const & color,
                                dp::TextureManager::ColorRegion const & outline,
                                gpu::TTextOutlinedStaticVertexBuffer & buffer)
    : m_colorCoord(glsl::ToVec2(color.GetTexRect().Center()))
    , m_outlineCoord(glsl::ToVec2(outline.GetTexRect().Center()))
    , m_buffer(buffer)
  {}

  void SetPenPosition(glsl::vec2 const & penOffset) {}

  void operator() (dp::TextureManager::GlyphRegion const & glyph, dp::text::GlyphMetrics const &)
  {
    m2::RectF const & mask = glyph.GetTexRect();
    m_buffer.emplace_back(m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.LeftTop()));
    m_buffer.emplace_back(m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.LeftBottom()));
    m_buffer.emplace_back(m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.RightTop()));
    m_buffer.emplace_back(m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.RightBottom()));
  }

protected:
  glsl::vec2 m_colorCoord;
  glsl::vec2 m_outlineCoord;
  gpu::TTextOutlinedStaticVertexBuffer & m_buffer;
};

struct LineMetrics
{
  size_t m_nextLineStartIndex;
  float m_scaledLength;  // In pixels.
  float m_scaledHeight;  // In pixels.
};

// Scan longer shaped glyphs, and try to split them into two strings if a space glyph is present.
// NOTE: Works only for LTR texts. Implementation should be mirrored for RTL.
buffer_vector<LineMetrics, 2> SplitText(bool forceNoWrap, float textScale, dp::GlyphFontAndId space, dp::text::TextMetrics const & str)
{
  // Add the whole line by default.
  buffer_vector<LineMetrics, 2> lines{{str.m_glyphs.size(),
      textScale * str.m_lineWidthInPixels, textScale * str.m_maxLineHeightInPixels}};

  size_t const count = str.m_glyphs.size();
  if (forceNoWrap || count <= 15)
    return lines;

  auto const begin = str.m_glyphs.begin();
  auto const end = str.m_glyphs.end();

  // Naive split on two parts using spaces as delimiters.
  // Doesn't take into an account the width of glyphs/string.
  auto const iMiddle = begin + count / 2;

  auto const isSpaceGlyph = [space](auto const & metrics){ return metrics.m_key == space; };
  // Find next delimiter after middle [m, e)
  auto iNext = std::find_if(iMiddle, end, isSpaceGlyph);

  // Find last delimiter before middle [b, m)
  auto iPrev = std::find_if(std::reverse_iterator(iMiddle), std::reverse_iterator(begin), isSpaceGlyph).base();
  // Don't split like this:
  //     xxxx
  // xxxxxxxxxxxx
  if (4 * (iPrev - begin) <= static_cast<long>(count))
    iPrev = end;
  else
    --iPrev;

  // Get the closest space to the middle.
  if (iNext == end || (iPrev != end && iMiddle - iPrev < iNext - iMiddle))
    iNext = iPrev;

  if (iNext == end)
    return lines;

  // Split string (actually, glyphs) into 2 parts.
  ASSERT(iNext != begin, ());
  ASSERT(space == iNext->m_key, ());

  auto const spaceIndex = iNext;
  auto const afterSpace = iNext + 1;
  ASSERT(afterSpace != end, ());

  lines.push_back(LineMetrics{
      count,
      textScale * std::accumulate(afterSpace, end, 0, [](auto acc, auto const & m){ return m.m_xAdvance + acc; }),
      textScale * str.m_maxLineHeightInPixels});

  // Update the first line too.
  lines[0].m_nextLineStartIndex = afterSpace - begin;
  auto const spaceWidth = textScale * spaceIndex->m_xAdvance;
  lines[0].m_scaledLength -= lines[1].m_scaledLength + spaceWidth;
  return lines;
}

class XLayouter
{
public:
  explicit XLayouter(dp::Anchor anchor)
    : m_anchor(anchor)
  {}

  float operator()(float currentLength, float maxLength) const
  {
    ASSERT_GREATER_OR_EQUAL(maxLength, currentLength, ());

    if (m_anchor & dp::Left)
      return 0.0;
    if (m_anchor & dp::Right)
      return -currentLength;

    return -(currentLength / 2.0f);
  }

private:
  dp::Anchor m_anchor;
};

class YLayouter
{
public:
  YLayouter(dp::Anchor anchor, float summaryHeight)
  {
    if (anchor & dp::Top)
      m_penOffset = 0.0f;
    else if (anchor & dp::Bottom)
      m_penOffset = -summaryHeight;
    else
      m_penOffset = -(summaryHeight / 2.0f);
  }

  float operator()(float currentHeight)
  {
    m_penOffset += currentHeight;
    return m_penOffset;
  }

private:
    float m_penOffset;
};

double GetTextMinPeriod(double pixelTextLength)
{
  double const vs = df::VisualParams::Instance().GetVisualScale();
  double const etalonEmpty = std::max(300.0 * vs, pixelTextLength);
  return etalonEmpty + pixelTextLength;
}
}  // namespace

ref_ptr<dp::Texture> TextLayout::GetMaskTexture() const
{
  ASSERT(!m_glyphRegions.empty(), ());
#ifdef DEBUG
  ref_ptr<dp::Texture> tex = m_glyphRegions[0].GetTexture();
  for (GlyphRegion const & g : m_glyphRegions)
    ASSERT(g.GetTexture() == tex, ());
#endif

  return m_glyphRegions[0].GetTexture();
}

size_t TextLayout::GetGlyphCount() const
{
  ASSERT_EQUAL(m_shapedGlyphs.m_glyphs.size(), m_glyphRegions.size(), ());
  return m_glyphRegions.size();
}

float TextLayout::GetPixelLength() const
{
  return m_shapedGlyphs.m_lineWidthInPixels * m_textSizeRatio;
}

float TextLayout::GetPixelHeight() const
{
  return m_shapedGlyphs.m_maxLineHeightInPixels * m_textSizeRatio;
}

dp::TGlyphs TextLayout::GetGlyphs() const
{
  // TODO(AB): Can conversion to TGlyphs be avoided?
  dp::TGlyphs glyphs;
  glyphs.reserve(m_shapedGlyphs.m_glyphs.size());
  for (auto const & glyph : m_shapedGlyphs.m_glyphs)
    glyphs.emplace_back(glyph.m_key);
  return glyphs;
}

StraightTextLayout::StraightTextLayout(std::string const & text, float fontSize, ref_ptr<dp::TextureManager> textures,
                                       dp::Anchor anchor, bool forceNoWrap)
{
  ASSERT_EQUAL(std::string::npos, text.find('\n'), ("Multiline text is not expected", text));

  m_textSizeRatio = fontSize * static_cast<float>(VisualParams::Instance().GetFontScale()) / dp::kBaseFontSizePixels;
  m_shapedGlyphs = textures->ShapeSingleTextLine(dp::kBaseFontSizePixels, text, &m_glyphRegions);

  // TODO(AB): Use ICU's BreakIterator to split text properly in different languages without spaces.
  // TODO(AB): Implement SplitText for RTL languages.
  auto const lines = SplitText(forceNoWrap || m_shapedGlyphs.m_isRTL, m_textSizeRatio, textures->GetSpaceGlyph(), m_shapedGlyphs);
  m_rowsCount = lines.size();

  float summaryHeight = 0.;
  float maxLength = 0;
  for (auto const & line : lines)
  {
    summaryHeight += line.m_scaledHeight;
    maxLength = std::max(maxLength, line.m_scaledLength);
  }

  XLayouter const xL(anchor);
  YLayouter yL(anchor, summaryHeight);

  for (auto const & l : lines)
    m_offsets.emplace_back(l.m_nextLineStartIndex, glsl::vec2(xL(l.m_scaledLength, maxLength), yL(l.m_scaledHeight)));

  m_pixelSize = m2::PointF(maxLength, summaryHeight);
}

m2::PointF StraightTextLayout::GetSymbolBasedTextOffset(m2::PointF const & symbolSize, dp::Anchor textAnchor,
                                                        dp::Anchor symbolAnchor)
{
  m2::PointF offset(0.0f, 0.0f);

  float const halfSymbolW = symbolSize.x / 2.0f;
  float const halfSymbolH = symbolSize.y / 2.0f;

  auto const adjustOffset = [&](dp::Anchor anchor)
  {
    if (anchor & dp::Top)
      offset.y += halfSymbolH;
    else if (anchor & dp::Bottom)
      offset.y -= halfSymbolH;

    if (anchor & dp::Left)
      offset.x += halfSymbolW;
    else if (anchor & dp::Right)
      offset.x -= halfSymbolW;
  };

  adjustOffset(textAnchor);
  adjustOffset(symbolAnchor);

  return offset;
}

glsl::vec2 StraightTextLayout::GetTextOffset(m2::PointF const & symbolSize, dp::Anchor textAnchor,
                                             dp::Anchor symbolAnchor) const
{
  auto const symbolBasedOffset = GetSymbolBasedTextOffset(symbolSize, textAnchor, symbolAnchor);
  return m_baseOffset + glsl::ToVec2(symbolBasedOffset);
}

void StraightTextLayout::CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                                             gpu::TTextStaticVertexBuffer & staticBuffer) const
{
  TextGeometryGenerator staticGenerator(colorRegion, staticBuffer);
  staticBuffer.reserve(4 * m_glyphRegions.size());
  Cache(staticGenerator);
}

void StraightTextLayout::CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                                             dp::TextureManager::ColorRegion const & outlineRegion,
                                             gpu::TTextOutlinedStaticVertexBuffer & staticBuffer) const
{
  TextOutlinedGeometryGenerator outlinedGenerator(colorRegion, outlineRegion, staticBuffer);
  staticBuffer.reserve(4 * m_glyphRegions.size());
  Cache(outlinedGenerator);
}

void StraightTextLayout::SetBasePosition(glm::vec4 const & pivot, glm::vec2 const & baseOffset)
{
  m_pivot = pivot;
  m_baseOffset = baseOffset;
}

void StraightTextLayout::CacheDynamicGeometry(glsl::vec2 const & pixelOffset,
                                              gpu::TTextDynamicVertexBuffer & dynamicBuffer) const
{
  StraightTextGeometryGenerator generator(m_pivot, pixelOffset, m_textSizeRatio, dynamicBuffer);
  dynamicBuffer.reserve(4 * m_glyphRegions.size());
  Cache(generator);
}

PathTextLayout::PathTextLayout(m2::PointD const & tileCenter, std::string const & text,
                               float fontSize, ref_ptr<dp::TextureManager> textureManager)
  : m_tileCenter(tileCenter)
{
  ASSERT_EQUAL(std::string::npos, text.find('\n'), ("Multiline text is not expected", text));

  auto const fontScale = static_cast<float>(VisualParams::Instance().GetFontScale());
  m_textSizeRatio = fontSize * fontScale / dp::kBaseFontSizePixels;

  // TODO(AB): StraightTextLayout used a logic to split a longer string into two strings.
  m_shapedGlyphs = textureManager->ShapeSingleTextLine(dp::kBaseFontSizePixels, text, &m_glyphRegions);
}

void PathTextLayout::CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                                         dp::TextureManager::ColorRegion const & outlineRegion,
                                         gpu::TTextOutlinedStaticVertexBuffer & staticBuffer) const
{
  TextOutlinedGeometryGenerator gen(colorRegion, outlineRegion, staticBuffer);
  staticBuffer.reserve(4 * m_glyphRegions.size());
  for (size_t i = 0; i < m_glyphRegions.size(); ++i)
    gen(m_glyphRegions[i], m_shapedGlyphs.m_glyphs[i]);
}

void PathTextLayout::CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                                         gpu::TTextStaticVertexBuffer & staticBuffer) const
{
  TextGeometryGenerator gen(colorRegion, staticBuffer);
  staticBuffer.reserve(4 * m_glyphRegions.size());
  for (size_t i = 0; i < m_glyphRegions.size(); ++i)
    gen(m_glyphRegions[i], m_shapedGlyphs.m_glyphs[i]);
}

bool PathTextLayout::CacheDynamicGeometry(m2::Spline::iterator const & iter, float depth,
                                          m2::PointD const & globalPivot,
                                          gpu::TTextDynamicVertexBuffer & buffer) const
{
  float const halfLength = 0.5f * GetPixelLength();

  m2::Spline::iterator beginIter = iter;
  beginIter.Advance(-halfLength);
  m2::Spline::iterator endIter = iter;
  endIter.Advance(halfLength);
  if (beginIter.BeginAgain() || endIter.BeginAgain())
    return false;

  float const halfFontSize = 0.5f * GetPixelHeight();
  float advanceSign = 1.0f;
  m2::Spline::iterator penIter = beginIter;
  if (beginIter.m_pos.x > endIter.m_pos.x)
  {
    advanceSign = -advanceSign;
    penIter = endIter;
  }

  m2::PointD const pxPivot = iter.m_pos;
  buffer.resize(4 * m_glyphRegions.size());

  glsl::vec4 const pivot(glsl::ToVec2(MapShape::ConvertToLocal(globalPivot, m_tileCenter,
                                                               kShapeCoordScalar)), depth, 0.0f);

  ASSERT_EQUAL(m_glyphRegions.size(), m_shapedGlyphs.m_glyphs.size(), ());
  for (size_t i = 0; i < m_glyphRegions.size(); ++i)
  {
    auto const & glyph = m_shapedGlyphs.m_glyphs[i];
    m2::PointF const pxSize = m_glyphRegions[i].GetPixelSize() * m_textSizeRatio;
    float const xAdvance = glyph.m_xAdvance * m_textSizeRatio;

    m2::PointD const baseVector = penIter.m_pos - pxPivot;
    m2::PointD const currentTangent = penIter.m_avrDir.Normalize();

    constexpr float kEps = 1e-5f;
    if (fabs(xAdvance) > kEps)
      penIter.Advance(advanceSign * xAdvance);
    m2::PointD const newTangent = penIter.m_avrDir.Normalize();

    glsl::vec2 const tangent = glsl::ToVec2(newTangent);
    glsl::vec2 const normal = glsl::vec2(-tangent.y, tangent.x);
    glsl::vec2 const formingVector = glsl::ToVec2(baseVector) + halfFontSize * normal;

    float const xOffset = glyph.m_xOffset * m_textSizeRatio;
    float const yOffset = glyph.m_yOffset * m_textSizeRatio;

    float const upVector = - (pxSize.y + yOffset);
    float const bottomVector = - yOffset;

    size_t const baseIndex = 4 * i;

    buffer[baseIndex + 0] = {pivot, formingVector + normal * bottomVector + tangent * xOffset};
    buffer[baseIndex + 1] = {pivot, formingVector + normal * upVector + tangent * xOffset};
    buffer[baseIndex + 2] = {pivot, formingVector + normal * bottomVector + tangent * (pxSize.x + xOffset)};
    buffer[baseIndex + 3] = {pivot, formingVector + normal * upVector + tangent * (pxSize.x + xOffset)};

    if (i > 0)
    {
      auto const dotProduct = static_cast<float>(m2::DotProduct(currentTangent, newTangent));
      if (dotProduct < kValidSplineTurn)
        return false;
    }
  }
  return true;
}

double PathTextLayout::CalculateTextLength(double textPixelLength)
{
  // We leave a little space on each side of the text.
  double constexpr kTextBorder = 4.0;
  return kTextBorder + textPixelLength;
}

void PathTextLayout::CalculatePositions(double splineLength, double splineScaleToPixel,
                                        double textPixelLength, std::vector<double> & offsets)
{
  double const textLength = CalculateTextLength(textPixelLength);

  // On the next scale m_scaleGtoP will be twice.
  if (textLength > splineLength * 2.0f * splineScaleToPixel)
    return;

  double constexpr kPathLengthScalar = 0.75;
  double const pathLength = kPathLengthScalar * splineScaleToPixel * splineLength;
  double const minPeriodSize = GetTextMinPeriod(textLength);
  double const twoTextsAndEmpty = minPeriodSize + textLength;

  if (pathLength < twoTextsAndEmpty)
  {
    // If we can't place 2 texts and empty part along the path,
    // we place only one text at the center of the path.
    offsets.push_back(splineLength * 0.5f);
  }
  else
  {
    double const textCount = std::max(floor(pathLength / minPeriodSize), 1.0);
    double const glbTextLen = splineLength / textCount;
    for (double offset = 0.5 * glbTextLen; offset < splineLength; offset += glbTextLen)
      offsets.push_back(offset);
  }
}

}  // namespace df
