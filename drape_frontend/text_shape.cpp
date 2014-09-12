#include "text_shape.hpp"
#include "common_structures.hpp"
#include "text_layout.hpp"
#include "fribidi.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

using m2::PointF;

namespace df
{

using m2::PointF;

namespace
{
float const TEXT_EXPAND_FACTOR = 1.3f;

PointF GetShift(dp::Anchor anchor, float width, float height)
{
  switch(anchor)
  {
  case dp::Center:      return PointF(-width / 2.0f, height / 2.0f);
  case dp::Left:        return PointF(0.0f, height / 2.0f);
  case dp::Right:       return PointF(-width, height / 2.0f);
  case dp::Top:         return PointF(-width / 2.0f, height);
  case dp::Bottom:      return PointF(-width / 2.0f, 0);
  case dp::LeftTop:     return PointF(0.0f, height);
  case dp::RightTop:    return PointF(-width, height);
  case dp::LeftBottom:  return PointF(0.0f, 0.0f);
  case dp::RightBottom: return PointF(-width, 0.0f);
  default:              return PointF(0.0f, 0.0f);
  }
}

void BatchText(dp::RefPointer<dp::Batcher> batcher, int32_t textureSet,
               vector<glsl_types::Quad4> const & positions,
               vector<glsl_types::Quad4> const & texCoord,
               vector<glsl_types::Quad4> const & fontColors,
               vector<glsl_types::Quad4> const & outlineColor,
               size_t glyphCount,
               dp::OverlayHandle * handle)
{
  ASSERT(glyphCount <= positions.size(), ());
  ASSERT(positions.size() == texCoord.size(), ());
  ASSERT(positions.size() == fontColors.size(), ());
  ASSERT(positions.size() == outlineColor.size(), ());

  dp::GLState state(gpu::FONT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetTextureSet(textureSet);
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(4, 4 * glyphCount);
  {
    dp::BindingInfo position(1);
    dp::BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, dp::MakeStackRefPointer((void*)&positions[0]));
  }
  {
    dp::BindingInfo texcoord(1);
    dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texcoord";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, texcoord, dp::MakeStackRefPointer((void*)&texCoord[0]));
  }
  {
    dp::BindingInfo base_color(1);
    dp::BindingDecl & decl = base_color.GetBindingDecl(0);
    decl.m_attributeName = "a_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, base_color, dp::MakeStackRefPointer((void*)&fontColors[0]));
  }
  {
    dp::BindingInfo outline_color(1);
    dp::BindingDecl & decl = outline_color.GetBindingDecl(0);
    decl.m_attributeName = "a_outline_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(3, outline_color, dp::MakeStackRefPointer((void*)&outlineColor[0]));
  }

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), MovePointer(handle), 4);
}

///Old code
void SplitText(strings::UniString const & visText,
              buffer_vector<strings::UniString, 3> & res,
              char const * delims,
              bool splitAllFound)
{
  if (!splitAllFound)
  {
    size_t count = visText.size();
    if (count > 15)
    {
      // split on two parts
      typedef strings::UniString::const_iterator TIter;
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
        res.push_back(strings::UniString(visText.begin(), iNext));

        if (++iNext != visText.end())
          res.push_back(strings::UniString(iNext, visText.end()));

        return;
      }
    }

    res.push_back(visText);
  }
  else
  {
    // split string using according to all delimiters
    typedef strings::SimpleDelimiter TDelim;
    for (strings::TokenizeIterator<TDelim> iter(visText, TDelim(delims)); iter; ++iter)
      res.push_back(iter.GetUniString());
  }
}

pair<bool, bool> GetBidiTexts(
    strings::UniString const & visText, strings::UniString const & auxVisText,
    strings::UniString & primaryText, strings::UniString & secondaryText)
{
  primaryText = fribidi::log2vis(visText);
  secondaryText = fribidi::log2vis(auxVisText);
  return make_pair(visText != primaryText, auxVisText != secondaryText);
}

void GetLayouts(vector<TextLayout> & layouts, strings::UniString const & text, bool isBidi, FontDecl const & fontDecl, dp::RefPointer<dp::TextureSetHolder> textures)
{
  if (!isBidi)
  {
    buffer_vector<strings::UniString, 3> res;
    SplitText(text, res, " \n\t", false);
    for (int i = 0; i < res.size(); ++i)
    {
      TextLayout tl = TextLayout(res[res.size() - i - 1], fontDecl, textures);
      if (tl.GetGlyphCount() == 0)
        continue;
      layouts.push_back(tl);
    }
  }
  else
  {
    TextLayout tl = TextLayout(text, fontDecl, textures);
    if (tl.GetGlyphCount() == 0)
      return;
    layouts.push_back(tl);
  }
}
}

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params)
  : m_basePoint(basePoint),
    m_params(params)
{
}

void TextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  ASSERT(!m_params.m_primaryText.empty(), ());
  vector<TextLayout> layouts;
  strings::UniString primaryText, secondaryText;
  pair<bool, bool> isBidi = GetBidiTexts(strings::MakeUniString(m_params.m_primaryText), strings::MakeUniString(m_params.m_secondaryText), primaryText, secondaryText);
  if (m_params.m_secondaryText.empty())
  {
    GetLayouts(layouts, primaryText, isBidi.first, m_params.m_primaryTextFont, textures);
    if (layouts.size() == 0)
      return;
    DrawMultipleLines(batcher, layouts, -1, textures);
  }
  else
  {
    GetLayouts(layouts, secondaryText, isBidi.second, m_params.m_secondaryTextFont, textures);
    int size = layouts.size() - 1;
    GetLayouts(layouts, primaryText, isBidi.first, m_params.m_primaryTextFont, textures);
    if (layouts.size() == 0)
      return;
    DrawMultipleLines(batcher, layouts, size, textures);
  }
}

void TextShape::DrawMultipleLines(dp::RefPointer<dp::Batcher> batcher, vector<TextLayout> & layouts,
                                  int delim, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  uint32_t const count = layouts.size();
  vector<float> lengths(count);
  float maxLength = 0.0f;
  float textHeight = 0.0f;
  uint32_t maxCount = 0;
  uint32_t symCount = 0;
  vector<uint32_t> heights(count);
  for (int i = 0; i < count; ++i)
  {
    lengths[i] = layouts[i].GetPixelLength();
    heights[i] = layouts[i].GetPixelHeight();
    textHeight += heights[i];
    maxLength = max(maxLength, lengths[i]);
    symCount += layouts[i].GetGlyphCount();
    maxCount = max(maxCount, layouts[i].GetGlyphCount());
  }

  PointF const anchorOffset = GetShift(m_params.m_anchor, maxLength, textHeight * TEXT_EXPAND_FACTOR);

  vector<glsl_types::Quad4> positions(symCount);
  vector<glsl_types::Quad4> texCoord(symCount);
  vector<glsl_types::Quad4> fontColor(symCount);
  vector<glsl_types::Quad4> indexes(symCount);

  float dy = (1.0f - TEXT_EXPAND_FACTOR) * heights[0];
  vector<PointF> pixelOffset(count);
  uint32_t delSymCount = 0;
  uint32_t lastIndex = 0;
  vector<TextLayout>::iterator it1;
  vector<PointF>::iterator it2;
  for (int i = 0; i < count; ++i)
  {
    float const dx = (maxLength - lengths[i]) / 2.0f;
    pixelOffset[i] = PointF(dx, dy) + anchorOffset + m_params.m_primaryOffset;
    dy -= heights[i] * TEXT_EXPAND_FACTOR;
    delSymCount += layouts[i].GetGlyphCount();
    if (i == delim)
    {
      it2 = pixelOffset.begin();
      it1 = layouts.begin();
      dp::OverlayHandle * handle = LayoutText(m_params.m_featureID, m_basePoint,
                                              it1, it2, m_params.m_depth,
                                              positions, texCoord, fontColor,
                                              indexes, textures, i + 1);

      BatchText(batcher, layouts[0].GetTextureSet(),
          positions, texCoord,
          fontColor, indexes,
          delSymCount, handle);

      delSymCount = 0;
      lastIndex = i + 1;
    }
  }
  it2 = pixelOffset.begin() + lastIndex;
  it1 = layouts.begin() + lastIndex;
  dp::OverlayHandle * handle = LayoutText(m_params.m_featureID, m_basePoint,
                                          it1, it2, m_params.m_depth,
                                          positions, texCoord, fontColor, indexes,
                                          textures, count - lastIndex);

  BatchText(batcher, layouts[0].GetTextureSet(),
            positions, texCoord,
            fontColor, indexes,
            delSymCount, handle);
}

} //end of df namespace
