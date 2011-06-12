#include "text_renderer.hpp"
#include "render_state.hpp"
#include "info_layer.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"

#include "../geometry/angles.hpp"

#include "../std/bind.hpp"

#include "../base/string_utils.hpp"


namespace yg
{
  namespace gl
  {

    TextRenderer::Params::Params()
      : m_drawTexts(true),
        m_glyphCacheID(0)
    {}

    TextRenderer::TextRenderer(Params const & params)
      : base_t(params),
        m_drawTexts(params.m_drawTexts),
        m_glyphCacheID(params.m_glyphCacheID)
    {}

    void TextRenderer::drawText(FontDesc const & fontDesc,
                                m2::PointD const & pt,
                                yg::EPosition pos,
                                float angle,
                                string const & utf8Text,
                                double depth,
                                bool log2vis)
    {
      if (!m_drawTexts)
        return;

      StraightTextElement::Params params;
      params.m_depth = depth;
      params.m_fontDesc = fontDesc;
      params.m_log2vis = log2vis;
      params.m_pivot = pt;
      params.m_position = pos;
      params.m_glyphCache = resourceManager()->glyphCache(m_glyphCacheID);
      params.m_logText = strings::MakeUniString(utf8Text);
     
      StraightTextElement ste(params);

      if (!renderState().get())
        ste.draw(this, math::Identity<double, 3>());
      else
        renderState()->m_currentInfoLayer->addStraightText(ste);
    }

    bool TextRenderer::drawPathText(
        FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
        double fullLength, double pathOffset, yg::EPosition pos, double depth)
    {
      if (!m_drawTexts)
        return false;

      PathTextElement::Params params;

      params.m_pts = path;
      params.m_ptsCount = s;
      params.m_fullLength = fullLength;
      params.m_pathOffset = pathOffset;
      params.m_fontDesc = fontDesc;
      params.m_logText = strings::FromUtf8(utf8Text);
      params.m_depth = depth;
      params.m_log2vis = true;
      params.m_glyphCache = resourceManager()->glyphCache(m_glyphCacheID);
      params.m_pivot = path[0];
      params.m_position = pos;

      PathTextElement pte(params);

      if (!renderState().get())
        pte.draw(this, math::Identity<double, 3>());
      else
        renderState()->m_currentInfoLayer->addPathText(pte);

      return true;
    }

    void TextRenderer::drawGlyph(m2::PointD const & ptOrg, m2::PointD const & ptGlyph, ang::AngleD const & angle, float /*blOffset*/, CharStyle const * p, double depth)
    {
      float x0 = ptGlyph.x + (p->m_xOffset - 1);
      float y1 = ptGlyph.y - (p->m_yOffset - 1);
      float y0 = y1 - (p->m_texRect.SizeY() - 2);
      float x1 = x0 + (p->m_texRect.SizeX() - 2);

      drawTexturedPolygon(ptOrg, angle,
                          p->m_texRect.minX() + 1,
                          p->m_texRect.minY() + 1,
                          p->m_texRect.maxX() - 1,
                          p->m_texRect.maxY() - 1,
                          x0, y0, x1, y1,
                          depth,
                          p->m_pageID);
    }

    GlyphCache * TextRenderer::glyphCache() const
    {
      return resourceManager()->glyphCache(m_glyphCacheID);
    }
  }
}
