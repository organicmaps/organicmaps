#include "graphics/text_renderer.hpp"
#include "graphics/overlay.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/glyph.hpp"

#include "geometry/angles.hpp"

#include "std/bind.hpp"

#include "base/string_utils.hpp"


namespace graphics
{
  //TextRenderer::Params::Params()
  //  : m_drawTexts(true)
  //{}

  TextRenderer::TextRenderer(base_t::Params const & params)
    : base_t(params)//, m_drawTexts(params.m_drawTexts)
  {}

  void TextRenderer::drawStraightGlyph(m2::PointD const & ptPivot,
                                       m2::PointD const & ptOffs,
                                       Glyph const * p,
                                       float depth)
  {
    float x0 = ptOffs.x + (p->m_info.m_metrics.m_xOffset - 1);
    float y1 = ptOffs.y - (p->m_info.m_metrics.m_yOffset - 1);
    float y0 = y1 - (p->m_texRect.SizeY() - 2);
    float x1 = x0 + (p->m_texRect.SizeX() - 2);

    drawStraightTexturedPolygon(
          ptPivot,
          p->m_texRect.minX() + 1,
          p->m_texRect.minY() + 1,
          p->m_texRect.maxX() - 1,
          p->m_texRect.maxY() - 1,
          x0, y0, x1, y1,
          depth,
          p->m_pipelineID
          );
  }

  void TextRenderer::drawGlyph(m2::PointD const & ptOrg,
                               m2::PointD const & ptGlyph,
                               ang::AngleD const & angle,
                               float /*blOffset*/,
                               Glyph const * p,
                               double depth)
  {
    float x0 = ptGlyph.x + (p->m_info.m_metrics.m_xOffset - 1);
    float y1 = ptGlyph.y - (p->m_info.m_metrics.m_yOffset - 1);
    float y0 = y1 - (p->m_texRect.SizeY() - 2);
    float x1 = x0 + (p->m_texRect.SizeX() - 2);

    drawTexturedPolygon(ptOrg, angle,
                        p->m_texRect.minX() + 1,
                        p->m_texRect.minY() + 1,
                        p->m_texRect.maxX() - 1,
                        p->m_texRect.maxY() - 1,
                        x0, y0, x1, y1,
                        depth,
                        p->m_pipelineID);
  }

  GlyphCache * TextRenderer::glyphCache() const
  {
    return resourceManager()->glyphCache(threadSlot());
  }
}
