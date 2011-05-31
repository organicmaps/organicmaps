#include "../base/SRC_FIRST.hpp"
#include "../base/string_utils.hpp"

#include "text_layout_element.hpp"
#include "glyph_cache.hpp"
#include "screen.hpp"
#include "resource_manager.hpp"
#include "resource_style.hpp"
#include "skin.hpp"

namespace yg
{
  TextLayoutElement::TextLayoutElement(
    char const * text,
    double depth,
    FontDesc const & fontDesc,
    bool log2vis,
    shared_ptr<Skin> const & skin,
    shared_ptr<ResourceManager> const & rm,
    m2::PointD const & pivot,
    yg::EPosition pos)
    : LayoutElement(0, pivot, pos),
      m_text(strings::FromUtf8(text)),
      m_depth(depth),
      m_fontDesc(fontDesc),
      m_skin(skin),
      m_rm(rm),
      m_log2vis(log2vis)
  {
    for (size_t i = 0; i < m_text.size(); ++i)
    {
      GlyphKey glyphKey(m_text[i], m_fontDesc.m_size, m_fontDesc.m_isMasked, m_fontDesc.m_color);

      if (m_fontDesc.m_isStatic)
      {
        uint32_t glyphID = m_skin->mapGlyph(glyphKey, m_fontDesc.m_isStatic);
        CharStyle const * p = static_cast<CharStyle const *>(m_skin->fromID(glyphID));
        if (p != 0)
        {
          if (i == 0)
            m_limitRect = m2::RectD(p->m_xOffset, p->m_yOffset, p->m_xOffset, p->m_yOffset);
          else
            m_limitRect.Add(m2::PointD(p->m_xOffset, p->m_yOffset));

          m_limitRect.Add(m2::PointD(p->m_xOffset + p->m_texRect.SizeX() - 4,
                                     p->m_yOffset + p->m_texRect.SizeY() - 4));

        }
      }
      else
      {
        GlyphMetrics const m = m_rm->getGlyphMetrics(glyphKey);
        if (i == 0)
          m_limitRect = m2::RectD(m.m_xOffset, m.m_yOffset, m.m_xOffset, m.m_yOffset);
        else
          m_limitRect.Add(m2::PointD(m.m_xOffset, m.m_yOffset));

        m_limitRect.Add(m2::PointD(m.m_xOffset + m.m_xAdvance,
                                   m.m_yOffset + m.m_yAdvance));
      }
    }

    /// centered by default
    m_limitRect.Offset(-m_limitRect.SizeX() / 2,
                       -m_limitRect.SizeY() / 2);

    /// adjusting according to position
    if (position() & EPosLeft)
      m_limitRect.Offset(-m_limitRect.SizeX() / 2, 0);
    if (position() & EPosRight)
      m_limitRect.Offset(m_limitRect.SizeX() / 2, 0);

    if (position() & EPosAbove)
      m_limitRect.Offset(0, m_limitRect.SizeY() / 2);

    if (position() & EPosUnder)
      m_limitRect.Offset(0, -m_limitRect.SizeY() / 2);
  }

  m2::RectD const TextLayoutElement::boundRect() const
  {
    return m_limitRect;
  }

  void TextLayoutElement::draw(Screen * /*screen*/)
  {
    /*
    yg::FontDesc desc = m_fontDesc;
    if (desc.m_isMasked)
    {
      desc.m_isMasked = false;
    }
    */
  }
}
