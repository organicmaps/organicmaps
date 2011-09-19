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

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m, FontDesc const & fontDesc, double depth) const
  {
    m2::PointD pv = layout.pivot() * m;

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      Skin * skin = screen->skin().get();
      GlyphLayoutElem const & elem = layout.entries()[i];
      GlyphKey glyphKey(elem.m_sym, fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);
      uint32_t const glyphID = skin->mapGlyph(glyphKey, screen->glyphCache());
      CharStyle const * charStyle = static_cast<CharStyle const *>(skin->fromID(glyphID));

      screen->drawGlyph(elem.m_pt + pv, m2::PointD(0.0, 0.0), elem.m_angle, 0, charStyle, depth);
    }
  }

  void TextElement::cacheTextImpl(GlyphLayout const & layout,
                                  StylesCache * stylesCache,
                                  FontDesc const & desc) const
  {
    vector<shared_ptr<SkinPage> > & skinPages = stylesCache->cachePages();
    GlyphCache * glyphCache = stylesCache->glyphCache();
    shared_ptr<ResourceManager> const & rm = stylesCache->resourceManager();

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      GlyphLayoutElem const & elem = layout.entries()[i];

      GlyphKey glyphKey(elem.m_sym,
                        m_fontDesc.m_size,
                        m_fontDesc.m_isMasked,
                        m_fontDesc.m_isMasked ? m_fontDesc.m_maskColor : m_fontDesc.m_color);

      bool found = false;

      for (unsigned j = 0; j < skinPages.size(); ++j)
        if (skinPages[j]->findGlyph(glyphKey) != 0x00FFFFFF)
        {
          found = true;
          break;
        }

      if (!found)
      {
        if (!skinPages.empty() && skinPages.back()->hasRoom(glyphKey, glyphCache))
          skinPages.back()->mapGlyph(glyphKey, glyphCache);
        else
        {
          skinPages.push_back(make_shared_ptr(new SkinPage(rm, SkinPage::EFontsUsage, 0)));
          skinPages.back()->mapGlyph(glyphKey, glyphCache);
        }
      }
    }
  }
}
