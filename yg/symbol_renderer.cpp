#include "../base/SRC_FIRST.hpp"
#include "symbol_renderer.hpp"
#include "skin.hpp"
#include "defines.hpp"

#include "../std/bind.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    SymbolRenderer::SymbolRenderer(base_t::Params const & params) : base_t(params)
    {}

    SymbolRenderer::SymbolObject::SymbolObject(m2::PointD const & pt, uint32_t styleID, EPosition pos, double depth)
      : m_pt(pt), m_pos(pos), m_styleID(styleID), m_depth(depth)
    {}

    m2::RectD const SymbolRenderer::SymbolObject::GetLimitRect(SymbolRenderer * p) const
    {
      ResourceStyle const * style = p->skin()->fromID(m_styleID);

      m2::RectU texRect(style->m_texRect);
      texRect.Inflate(-1, -1);

      m2::PointD posPt = p->getPosPt(m_pt, m2::RectD(texRect), m_pos);

      return m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY()));
    }

    void SymbolRenderer::SymbolObject::Draw(SymbolRenderer * p) const
    {
      p->drawSymbolImpl(m_pt, m_styleID, m_pos, m_depth);
    }

    m2::PointD const SymbolRenderer::getPosPt(m2::PointD const & pt, m2::RectD const & texRect, EPosition pos)
    {
      m2::PointD posPt;

      if (pos & EPosLeft)
        posPt.x = pt.x - texRect.SizeX();
      else if (pos & EPosRight)
        posPt.x = pt.x;
      else
        posPt.x = pt.x - texRect.SizeX() / 2;

      if (pos & EPosAbove)
        posPt.y = pt.y - texRect.SizeY();
      else if (pos & EPosUnder)
        posPt.y = pt.y;
      else
        posPt.y = pt.y - texRect.SizeY() / 2;

      return posPt;
    }

    void SymbolRenderer::drawSymbolImpl(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth)
    {
      ResourceStyle const * style(skin()->fromID(styleID));
      if (style == 0)
      {
        LOG(LINFO, ("styleID=", styleID, " wasn't found on the current skin"));
        return;
      }

      m2::RectU texRect(style->m_texRect);
      texRect.Inflate(-1, -1);

      m2::PointD posPt = getPosPt(pt, m2::RectD(texRect), pos);

      drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          depth,
                          style->m_pageID);
    }

    void mark_intersect(bool & flag)
    {
      flag = true;
    }

    void SymbolRenderer::drawSymbol(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth)
    {
      ResourceStyle const * style(skin()->fromID(styleID));
      if (style == 0)
      {
        LOG(LINFO, ("styleID=", styleID, " wasn't found on the current skin"));
        return;
      }

      SymbolObject obj(pt, styleID, pos, depth);
      m2::RectD r = obj.GetLimitRect(this);

      bool isIntersect = false;

      m_symbolsMap[styleID].ForEachInRect(obj.GetLimitRect(this), bind(&mark_intersect, ref(isIntersect)));

      if (!isIntersect)
        m_symbolsMap[styleID].Add(obj, r);
    }

    void SymbolRenderer::drawCircle(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth)
    {
      drawSymbolImpl(pt, styleID, pos, yg::maxDepth);
    }

    void SymbolRenderer::setClipRect(m2::RectI const & rect)
    {
      for (symbols_map_t::const_iterator it = m_symbolsMap.begin(); it != m_symbolsMap.end(); ++it)
        it->second.ForEach(bind(&SymbolObject::Draw, _1, this));
      m_symbolsMap.clear();

      base_t::setClipRect(rect);
    }

    void SymbolRenderer::endFrame()
    {
      for (symbols_map_t::const_iterator it = m_symbolsMap.begin(); it != m_symbolsMap.end(); ++it)
        it->second.ForEach(bind(&SymbolObject::Draw, _1, this));
      m_symbolsMap.clear();

      base_t::endFrame();
    }

  }
}
