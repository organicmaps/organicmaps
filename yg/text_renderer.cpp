#include "../base/SRC_FIRST.hpp"

#include "defines.hpp"
#include "text_renderer.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "skin.hpp"
#include "render_state.hpp"

#include "../coding/strutil.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"


namespace yg
{
  namespace gl
  {
    TextRenderer::Params::Params() : m_useTextLayer(false)
    {
    }

    TextRenderer::TextRenderer(Params const & params)
      : base_t(params), m_useTextLayer(params.m_useTextLayer)
    {}

    void TextRenderer::TextObj::Draw(GeometryBatcher * pBatcher) const
    {
      pBatcher->drawText(m_pt, 0.0, m_size, m_utf8Text, yg::maxDepth, m_isFixedFont, m_log2vis);
    }

    m2::RectD const TextRenderer::TextObj::GetLimitRect(GeometryBatcher * pBatcher) const
    {
      return m2::Offset(pBatcher->textRect(m_utf8Text, m_size, false, m_log2vis), m_pt);
    }

    void TextRenderer::drawText(m2::PointD const & pt,
                  float angle,
                  uint8_t fontSize,
                  string const & utf8Text,
                  double depth,
                  bool isFixedFont,
                  bool log2vis)
    {
      if (isFixedFont)
        base_t::drawText(pt, angle, fontSize, utf8Text, depth, isFixedFont, log2vis);
      else
      {
        TextObj obj(pt, utf8Text, fontSize, depth, isFixedFont, log2vis);
        m2::RectD r = obj.GetLimitRect(this);
  /*
        m2::PointD pts[5] =
        {
          m2::PointD(r.minX(), r.minY()),
          m2::PointD(r.minX(), r.maxY()),
          m2::PointD(r.maxX(), r.maxY()),
          m2::PointD(r.maxX(), r.minY()),
          m2::PointD(r.minX(), r.minY())
        };

        drawPath(pts, 5, skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 255), 2, 0, 0, 0)), depth);
   */

        m_tree.ReplaceIf(obj, r, TextObj::better_depth());
      }
    }

    void TextRenderer::setClipRect(m2::RectI const & rect)
    {
      m_tree.ForEach(bind(&TextObj::Draw, _1, this));
      m_tree.Clear();
      base_t::setClipRect(rect);
    }

    void TextRenderer::endFrame()
    {
      m_tree.ForEach(bind(&TextObj::Draw, _1, this));
      m_tree.Clear();

      base_t::endFrame();
    }
  }
}
