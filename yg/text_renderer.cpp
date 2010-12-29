#include "../base/SRC_FIRST.hpp"

#include "text_renderer.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"

#include "../coding/strutil.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"


namespace yg { namespace gl {

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
  m_tree.ReplaceIf(obj, obj.GetLimitRect(this), TextObj::better_depth());
}

void TextRenderer::endFrame()
{
  m_tree.ForEach(bind(&TextObj::Draw, _1, this));
  m_tree.Clear();

  GeometryBatcher::endFrame();
}

}}
