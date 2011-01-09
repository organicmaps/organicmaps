#include "../base/SRC_FIRST.hpp"

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
      pBatcher->drawText(m_pt, 0.0, m_size, m_utf8Text, m_depth);
    }

    m2::RectD TextRenderer::TextObj::GetLimitRect(GeometryBatcher * pBatcher) const
    {
      shared_ptr<ResourceManager> pRM = pBatcher->resourceManager();
      m2::RectD rect;
      m2::PointD pt(0, 0);

      wstring const text = FromUtf8(m_utf8Text);
      for (size_t i = 0; i < text.size(); ++i)
      {
        GlyphMetrics const m = pRM->getGlyphMetrics(GlyphKey(text[i], m_size, true));

        rect.Add(m_pt + pt);
        rect.Add(m_pt + pt + m2::PointD(m.m_xOffset + m.m_width, - m.m_yOffset - m.m_height));
        pt += m2::PointD(m.m_xAdvance, 0);
      }

      rect.Inflate(2, 2);
      return rect;
    }

    void TextRenderer::drawText(m2::PointD const & pt,
                  float angle,
                  uint8_t fontSize,
                  string const & utf8Text,
                  double depth)
    {
      TextObj obj(pt, utf8Text, fontSize, depth);
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

    void TextRenderer::endFrame()
    {
      shared_ptr<RenderTarget> rt;

      m_tree.ForEach(bind(&TextObj::Draw, _1, this));
      m_tree.Clear();

      base_t::endFrame();
    }
  }
}
