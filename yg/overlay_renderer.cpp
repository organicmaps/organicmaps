#include "../base/SRC_FIRST.hpp"

#include "overlay_renderer.hpp"
#include "text_element.hpp"
#include "symbol_element.hpp"
#include "render_state.hpp"
#include "info_layer.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"

namespace yg
{
  namespace gl
  {
    OverlayRenderer::Params::Params()
      : m_useOverlay(false),
        m_drawTexts(true),
        m_drawSymbols(true)
    {
    }

    OverlayRenderer::OverlayRenderer(Params const & p)
      : TextRenderer(p),
        m_useOverlay(p.m_useOverlay),
        m_drawTexts(p.m_drawTexts),
        m_drawSymbols(p.m_drawSymbols),
        m_infoLayer(p.m_infoLayer)
    {
    }

    void OverlayRenderer::drawSymbol(m2::PointD const & pt,
                                     string const & symbolName,
                                     EPosition pos,
                                     int depth)
    {
      if (!m_drawSymbols)
        return;

      SymbolElement::Params params;

      params.m_depth = depth;
      params.m_position = pos;
      params.m_pivot = pt;
      params.m_symbolName = symbolName;
      params.m_styleID = 0;
      params.m_skin = skin().get();

      SymbolElement se(params);

      if (!m_infoLayer.get() || !m_useOverlay)
        se.draw(this, math::Identity<double, 3>());
      else
        m_infoLayer->addSymbol(se, math::Identity<double, 3>());
    }

    void OverlayRenderer::drawCircle(m2::PointD const & pt,
                                     uint32_t styleID,
                                     EPosition pos,
                                     int depth)
    {
      SymbolElement::Params params;
      params.m_depth = depth;
      params.m_position = pos;
      params.m_pivot = pt;
      params.m_styleID = styleID;
      params.m_skin = skin().get();

      SymbolElement se(params);

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      se.draw(this, id);
    }

    void OverlayRenderer::drawText(FontDesc const & fontDesc,
                                m2::PointD const & pt,
                                yg::EPosition pos,
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
      params.m_glyphCache = glyphCache();
      params.m_logText = strings::MakeUniString(utf8Text);

      StraightTextElement ste(params);

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get() || !m_useOverlay)
        ste.draw(this, id);
      else
        m_infoLayer->addStraightText(ste, id);
    }

    bool OverlayRenderer::drawPathText(
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
      params.m_logText = strings::MakeUniString(utf8Text);
      params.m_depth = depth;
      params.m_log2vis = true;
      params.m_glyphCache = glyphCache();
      params.m_pivot = path[0];
      params.m_position = pos;

      PathTextElement pte(params);

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get() || !m_useOverlay)
        pte.draw(this, id);
      else
        m_infoLayer->addPathText(pte, id);

      return true;
    }

    void OverlayRenderer::setInfoLayer(shared_ptr<InfoLayer> const & infoLayer)
    {
      m_infoLayer = infoLayer;
    }

    shared_ptr<InfoLayer> const & OverlayRenderer::infoLayer() const
    {
      return m_infoLayer;
    }

    void OverlayRenderer::resetInfoLayer()
    {
      m_infoLayer.reset();
    }
  }
}
