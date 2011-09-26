#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "skin_page.hpp"
#include "styles_cache.hpp"
#include "resource_manager.hpp"
#include "overlay_renderer.hpp"
#include "resource_style.hpp"

#include "../base/logging.hpp"

namespace yg
{
  TextElement::TextElement(Params const & p)
    : OverlayElement(p),
      m_fontDesc(p.m_fontDesc),
      m_logText(p.m_logText),
      m_log2vis(p.m_log2vis),
      m_glyphCache(p.m_glyphCache)
  {
    if (m_log2vis)
      m_visText = m_glyphCache->log2vis(m_logText);
    else
      m_visText = m_logText;
  }

  strings::UniString const & TextElement::logText() const
  {
    return m_logText;
  }

  strings::UniString const & TextElement::visText() const
  {
    return m_visText;
  }

  FontDesc const & TextElement::fontDesc() const
  {
    return m_fontDesc;
  }

  bool TextElement::isBidi() const
  {
    return m_logText != m_visText;
  }

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m, FontDesc const & fontDesc, double depth) const
  {
    m2::PointD pv = layout.pivot() * m;

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      Skin * skin = screen->skin().get();
      GlyphLayoutElem const & elem = layout.entries()[i];
      GlyphKey glyphKey(elem.m_sym,
                        fontDesc.m_size,
                        fontDesc.m_isMasked,
                        fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);
      uint32_t const glyphID = skin->mapGlyph(glyphKey, screen->glyphCache());
      CharStyle const * charStyle = static_cast<CharStyle const *>(skin->fromID(glyphID));

      screen->drawGlyph(elem.m_pt + pv, m2::PointD(0.0, 0.0), elem.m_angle, 0, charStyle, depth);
    }
  }

  void TextElement::map(GlyphLayout const & layout,
                        StylesCache * stylesCache,
                        FontDesc const & desc) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();
    GlyphCache * glyphCache = stylesCache->glyphCache();

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      GlyphLayoutElem const & elem = layout.entries()[i];

      GlyphKey glyphKey(elem.m_sym,
                        desc.m_size,
                        desc.m_isMasked,
                        desc.m_isMasked ? desc.m_maskColor : desc.m_color);

      bool packed = skinPage->findGlyph(glyphKey) != 0x00FFFFFF;

      if (!packed)
      {
        if (skinPage->hasRoom(glyphKey, glyphCache))
        {
          skinPage->mapGlyph(glyphKey, glyphCache);
          packed = true;
        }
      }

      CHECK(packed, ("couldn't pack"));
    }
  }

  bool TextElement::find(GlyphLayout const & layout, StylesCache * stylesCache, FontDesc const & desc) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      GlyphLayoutElem const & elem = layout.entries()[i];

      GlyphKey glyphKey(elem.m_sym,
                        desc.m_size,
                        desc.m_isMasked,
                        desc.m_isMasked ? desc.m_maskColor : desc.m_color);

      if (skinPage->findGlyph(glyphKey) == 0x00FFFFFF)
        return false;
    }

    return true;
  }

  void TextElement::fillUnpacked(GlyphLayout const & layout,
                                 FontDesc const & desc,
                                 StylesCache * stylesCache,
                                 vector<m2::PointU> & v) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();
    GlyphCache * glyphCache = stylesCache->glyphCache();

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      GlyphLayoutElem const & elem = layout.entries()[i];

      GlyphKey glyphKey(elem.m_sym,
                        desc.m_size,
                        desc.m_isMasked,
                        desc.m_isMasked ? desc.m_maskColor : desc.m_color);

      if (skinPage->findGlyph(glyphKey) == 0x00FFFFFF)
      {
        shared_ptr<GlyphInfo> gi = glyphCache->getGlyphInfo(glyphKey);
        v.push_back(m2::PointU(gi->m_metrics.m_width + 4, gi->m_metrics.m_height + 4));
      }
    }
  }
}
