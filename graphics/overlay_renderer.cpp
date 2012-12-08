#include "../base/SRC_FIRST.hpp"

#include "overlay_renderer.hpp"
#include "composite_overlay_element.hpp"
#include "straight_text_element.hpp"
#include "path_text_element.hpp"
#include "symbol_element.hpp"
#include "circle_element.hpp"
#include "overlay.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"

namespace graphics
{
  OverlayRenderer::Params::Params()
    : m_drawTexts(true),
      m_drawSymbols(true)
  {
  }

  OverlayRenderer::OverlayRenderer(Params const & p)
    : TextRenderer(p),
      m_drawTexts(p.m_drawTexts),
      m_drawSymbols(p.m_drawSymbols),
      m_overlay(p.m_overlay)
  {
  }

  void OverlayRenderer::drawSymbol(SymbolElement::Params & params)
  {
    if (!m_drawSymbols)
      return;

    params.m_skin = skin().get();

    shared_ptr<OverlayElement> oe(new SymbolElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  void OverlayRenderer::drawSymbol(m2::PointD const & pt, string const & name, EPosition pos, int depth)
  {
    graphics::SymbolElement::Params params;
    params.m_depth = depth;
    params.m_position = pos;
    params.m_pivot = pt;
    params.m_info.m_name = name;

    drawSymbol(params);
  }

  void OverlayRenderer::drawCircle(m2::PointD const & pt,
                                   graphics::Circle::Info const & ci,
                                   EPosition pos,
                                   int depth)
  {
    CircleElement::Params params;

    params.m_depth = depth;
    params.m_position = pos;
    params.m_pivot = pt;
    params.m_ci = ci;

    shared_ptr<OverlayElement> oe(new CircleElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  void OverlayRenderer::drawText(FontDesc const & fontDesc,
                                 m2::PointD const & pt,
                                 graphics::EPosition pos,
                                 string const & utf8Text,
                                 double depth,
                                 bool log2vis,
                                 bool doSplit)
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
    params.m_doSplit = doSplit;
    params.m_useAllParts = false;

    shared_ptr<OverlayElement> oe(new StraightTextElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  void OverlayRenderer::drawTextEx(FontDesc const & primaryFont,
                                   FontDesc const & secondaryFont,
                                   m2::PointD const & pt,
                                   graphics::EPosition pos,
                                   string const & text,
                                   string const & secondaryText,
                                   double depth,
                                   bool log2vis,
                                   bool doSplit)
  {
    if (!m_drawTexts)
      return;

    StraightTextElement::Params params;
    params.m_depth = depth;
    params.m_fontDesc = primaryFont;
    params.m_auxFontDesc = secondaryFont;
    params.m_log2vis = log2vis;
    params.m_pivot = pt;
    params.m_position = pos;
    params.m_glyphCache = glyphCache();
    params.m_logText = strings::MakeUniString(text);
    params.m_auxLogText = strings::MakeUniString(secondaryText);
    params.m_doSplit = doSplit;
    params.m_useAllParts = false;

    shared_ptr<OverlayElement> oe(new StraightTextElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  bool OverlayRenderer::drawPathText(
      FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
      double fullLength, double pathOffset, graphics::EPosition pos, double depth)
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

    shared_ptr<PathTextElement> pte(new PathTextElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      pte->draw(this, id);
    else
      m_overlay->processOverlayElement(pte);

    return true;
  }

  void OverlayRenderer::setOverlay(shared_ptr<Overlay> const & overlay)
  {
    m_overlay = overlay;
  }

  shared_ptr<Overlay> const & OverlayRenderer::overlay() const
  {
    return m_overlay;
  }

  void OverlayRenderer::resetOverlay()
  {
    m_overlay.reset();
  }
}
