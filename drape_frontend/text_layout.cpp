#include "text_layout.hpp"
#include "fribidi.hpp"

#include "../drape/glsl_func.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/numeric.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

namespace df
{

namespace
{

float const BASE_HEIGHT = 20.0f;

class TextGeometryGenerator
{
public:
  TextGeometryGenerator(glsl::vec3 const & pivot,
                        dp::TextureManager::ColorRegion const & color,
                        dp::TextureManager::ColorRegion const & outline,
                        gpu::TTextStaticVertexBuffer & buffer)
    : m_depthShift(0.0f, 0.0f, 0.0f)
    , m_pivot(pivot)
    , m_colorCoord(glsl::ToVec2(color.GetTexRect().Center()))
    , m_outlineCoord(glsl::ToVec2(outline.GetTexRect().Center()))
    , m_buffer(buffer)
  {
  }

  void operator() (dp::TextureManager::GlyphRegion const & glyph)
  {
    m2::RectF const & mask = glyph.GetTexRect();
    glsl::vec3 pivot = m_pivot + m_depthShift;
    m_buffer.push_back(gpu::TextStaticVertex(pivot, m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.LeftBottom())));
    m_buffer.push_back(gpu::TextStaticVertex(pivot, m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.LeftTop())));
    m_buffer.push_back(gpu::TextStaticVertex(pivot, m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.RightBottom())));
    m_buffer.push_back(gpu::TextStaticVertex(pivot, m_colorCoord, m_outlineCoord, glsl::ToVec2(mask.RightTop())));
    m_depthShift += glsl::vec3(0.0f, 0.0f, 1.0f);
  }

protected:
  glsl::vec3 m_depthShift;
  glsl::vec3 const & m_pivot;
  glsl::vec2 m_colorCoord;
  glsl::vec2 m_outlineCoord;
  gpu::TTextStaticVertexBuffer & m_buffer;
};

class StraigthTextGeometryGenerator : public TextGeometryGenerator
{
  typedef TextGeometryGenerator TBase;
public:
  StraigthTextGeometryGenerator(glsl::vec3 const & pivot, glsl::vec2 const & pixelOffset,
                                float textRatio,
                                dp::TextureManager::ColorRegion const & color,
                                dp::TextureManager::ColorRegion const & outline,
                                gpu::TTextStaticVertexBuffer & staticBuffer,
                                gpu::TTextDynamicVertexBuffer & dynBuffer)
    : TBase(pivot, color, outline, staticBuffer)
    , m_penPosition(pixelOffset)
    , m_buffer(dynBuffer)
    , m_textRatio(textRatio)
  {
  }

  void operator()(dp::TextureManager::GlyphRegion const & glyph)
  {
    m2::PointU pixelSize(m2::PointU::Zero());
    glyph.GetPixelSize(pixelSize);
    pixelSize *= m_textRatio;

    float const xOffset = glyph.GetOffsetX() * m_textRatio;
    float const yOffset = glyph.GetOffsetY() * m_textRatio;

    float const upVector = -static_cast<int32_t>(pixelSize.y) - yOffset;
    float const bottomVector = -yOffset;

    m_buffer.push_back(gpu::TextDynamicVertex(m_penPosition + glsl::vec2(xOffset, upVector)));
    m_buffer.push_back(gpu::TextDynamicVertex(m_penPosition + glsl::vec2(xOffset, bottomVector)));
    m_buffer.push_back(gpu::TextDynamicVertex(m_penPosition + glsl::vec2(pixelSize.x + xOffset, upVector)));
    m_buffer.push_back(gpu::TextDynamicVertex(m_penPosition + glsl::vec2(pixelSize.x + xOffset, bottomVector)));

    m_penPosition += glsl::vec2(glyph.GetAdvanceX() * m_textRatio, glyph.GetAdvanceY() * m_textRatio);

    TBase::operator()(glyph);
  }

private:
  glsl::vec2 m_penPosition;
  gpu::TTextDynamicVertexBuffer & m_buffer;
  float m_textRatio = 0.0;
};

///Old code
void SplitText(strings::UniString & visText,
               buffer_vector<size_t, 2> & delimIndexes)
{
  char const * delims = " \n\t";
  size_t count = visText.size();
  if (count > 15)
  {
    // split on two parts
    typedef strings::UniString::iterator TIter;
    TIter const iMiddle = visText.begin() + count/2;

    size_t const delimsSize = strlen(delims);

    // find next delimeter after middle [m, e)
    TIter iNext = find_first_of(iMiddle,
                                visText.end(),
                                delims, delims + delimsSize);

    // find last delimeter before middle [b, m)
    TIter iPrev = find_first_of(reverse_iterator<TIter>(iMiddle),
                                reverse_iterator<TIter>(visText.begin()),
                                delims, delims + delimsSize).base();
    // don't do split like this:
    //     xxxx
    // xxxxxxxxxxxx
    if (4 * distance(visText.begin(), iPrev) <= count)
      iPrev = visText.end();
    else
      --iPrev;

    // get closest delimiter to the middle
    if (iNext == visText.end() ||
        (iPrev != visText.end() && distance(iPrev, iMiddle) < distance(iMiddle, iNext)))
    {
      iNext = iPrev;
    }

    // split string on 2 parts
    if (iNext != visText.end())
    {
      ASSERT_NOT_EQUAL(iNext, visText.begin(), ());
      TIter delimSymbol = iNext;
      TIter secondPart = iNext + 1;

      delimIndexes.push_back(distance(visText.begin(), delimSymbol));

      if (secondPart != visText.end())
      {
        strings::UniString result(visText.begin(), delimSymbol);
        result.append(secondPart, visText.end());
        visText = result;
        delimIndexes.push_back(visText.size());
      }

      return;
    }
  }

  delimIndexes.push_back(count);
}

class XLayouter
{
public:
  XLayouter(dp::Anchor anchor)
    : m_anchor(anchor)
  {
  }

  float operator()(float currentLength, float maxLength)
  {
    ASSERT_GREATER_OR_EQUAL(maxLength, currentLength, ());

    if (m_anchor & dp::Left)
      return 0.0;
    else if (m_anchor & dp::Right)
      return -currentLength;
    else
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

void CalculateOffsets(dp::Anchor anchor,
                      float textRatio,
                      dp::TextureManager::TGlyphsBuffer const & glyphs,
                      buffer_vector<size_t, 2> const & delimIndexes,
                      buffer_vector<pair<size_t, glsl::vec2>, 2> & result,
                      m2::PointU & pixelSize)
{
  typedef pair<float, float> TLengthAndHeight;
  buffer_vector<TLengthAndHeight, 2> lengthAndHeight;
  float maxLength = 0;
  float summaryHeight = 0;

  size_t start = 0;
  for (size_t index = 0; index < delimIndexes.size(); ++index)
  {
    size_t end = delimIndexes[index];
    ASSERT_NOT_EQUAL(start, end, ());
    lengthAndHeight.push_back(TLengthAndHeight(0, 0));
    TLengthAndHeight & node = lengthAndHeight.back();
    for (size_t glyphIndex = start; glyphIndex < end; ++glyphIndex)
    {
      dp::TextureManager::GlyphRegion const & glyph = glyphs[glyphIndex];
      node.first += (glyph.GetAdvanceX() * textRatio);
      node.second = max(node.second, (glyph.GetPixelHeight() + glyph.GetAdvanceY()) * textRatio);
    }
    maxLength = max(maxLength, node.first);
    summaryHeight += node.second;
    start = end;
  }

  ASSERT_EQUAL(delimIndexes.size(), lengthAndHeight.size(), ());

  XLayouter xL(anchor);
  YLayouter yL(anchor, summaryHeight);
  for (size_t index = 0; index < delimIndexes.size(); ++index)
  {
    TLengthAndHeight const & node = lengthAndHeight[index];
    result.push_back(make_pair(delimIndexes[index], glsl::vec2(xL(node.first, maxLength),
                                                               yL(node.second))));
  }

  pixelSize = m2::PointU(maxLength, summaryHeight);
}

} // namespace

TextLayout::TextLayout(df::TextLayout::LayoutType type)
  : m_type(type)
{
}

void TextLayout::Init(strings::UniString const & text, float fontSize,
                      dp::RefPointer<dp::TextureManager> textures)
{
  m_textSizeRatio = fontSize / BASE_HEIGHT;
  textures->GetGlyphRegions(text, m_metrics);
}

dp::RefPointer<dp::Texture> TextLayout::GetMaskTexture() const
{
  ASSERT(!m_metrics.empty(), ());
#ifdef DEBUG
  dp::RefPointer<dp::Texture> tex = m_metrics[0].GetTexture();
  for (GlyphRegion const & g : m_metrics)
    ASSERT(g.GetTexture() == tex, ());
#endif

  return m_metrics[0].GetTexture();
}

uint32_t TextLayout::GetGlyphCount() const
{
  return m_metrics.size();
}

float TextLayout::GetPixelLength() const
{
  return m_textSizeRatio * accumulate(m_metrics.begin(), m_metrics.end(), 0.0, [](double const & v, GlyphRegion const & glyph)
  {
    return v + glyph.GetAdvanceX();
  });
}

float TextLayout::GetPixelHeight() const
{
  return m_textSizeRatio * BASE_HEIGHT;
}

StraightTextLayout::StraightTextLayout(strings::UniString const & text, float fontSize,
                                       dp::RefPointer<dp::TextureManager> textures, dp::Anchor anchor)
  : TBase(TextLayout::LayoutType::StraightLayout)
{
  strings::UniString visibleText = fribidi::log2vis(text);
  buffer_vector<size_t, 2> delimIndexes;
  if (visibleText == text)
    SplitText(visibleText, delimIndexes);
  else
    delimIndexes.push_back(visibleText.size());

  TBase::Init(visibleText, fontSize, textures);
  CalculateOffsets(anchor, m_textSizeRatio, m_metrics, delimIndexes, m_offsets, m_pixelSize);
}

void StraightTextLayout::Cache(glm::vec3 const & pivot, glm::vec2 const & pixelOffset,
                               dp::TextureManager::ColorRegion const & colorRegion,
                               dp::TextureManager::ColorRegion const & outlineRegion,
                               gpu::TTextStaticVertexBuffer & staticBuffer,
                               gpu::TTextDynamicVertexBuffer & dynamicBuffer) const
{
  size_t beginOffset = 0;
  for (pair<size_t, glsl::vec2> node : m_offsets)
  {
    size_t endOffset = node.first;
    StraigthTextGeometryGenerator generator(pivot, pixelOffset + node.second, m_textSizeRatio,
                                            colorRegion, outlineRegion, staticBuffer, dynamicBuffer);
    for (size_t index = beginOffset; index < endOffset; ++index)
      generator(m_metrics[index]);

    beginOffset = endOffset;
  }
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

} // namespace df
