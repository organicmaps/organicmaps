#include "overlay_renderer.hpp"
#include "straight_text_element.hpp"
#include "path_text_element.hpp"
#include "symbol_element.hpp"
#include "circle_element.hpp"
#include "circled_symbol.hpp"
#include "overlay.hpp"
#include "resource_manager.hpp"


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
    params.m_renderer = this;

    drawSymbol(params);
  }

  void OverlayRenderer::drawCircle(CircleElement::Params & params)
  {
    shared_ptr<OverlayElement> oe(new CircleElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  void OverlayRenderer::drawCircle(m2::PointD const & pt,
                                   graphics::Circle::Info const & ci,
                                   EPosition pos,
                                   double depth)
  {
    CircleElement::Params params;

    params.m_depth = depth;
    params.m_position = pos;
    params.m_pivot = pt;
    params.m_ci = ci;

    drawCircle(params);
  }

  void OverlayRenderer::drawCircledSymbol(SymbolElement::Params const & symParams,
                         CircleElement::Params const & circleParams)
  {
    shared_ptr<OverlayElement> oe(new CircledSymbol(symParams, circleParams));

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
    params.m_logText = strings::MakeUniString(utf8Text);
    params.m_doSplit = doSplit;
    params.m_useAllParts = false;
    params.m_offset = m2::PointD(0,0);
    params.m_glyphCache = glyphCache();

    shared_ptr<OverlayElement> oe(new StraightTextElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      oe->draw(this, id);
    else
      m_overlay->processOverlayElement(oe);
  }

  void OverlayRenderer::drawTextEx(StraightTextElement::Params & params)
  {
    if (!m_drawTexts)
      return;

    params.m_glyphCache = glyphCache();

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
    params.m_logText = strings::MakeUniString(text);
    params.m_auxLogText = strings::MakeUniString(secondaryText);
    params.m_doSplit = doSplit;
    params.m_useAllParts = false;

    drawTextEx(params);
  }

  void OverlayRenderer::drawPathText(FontDesc const & fontDesc,
                                     m2::PointD const * path,
                                     size_t pathSize,
                                     string const & utf8Text,
                                     double fullLength,
                                     double pathOffset,
                                     double textOffset,
                                     double depth)
  {
    if (!m_drawTexts)
      return;

    PathTextElement::Params params;

    params.m_pts = path;
    params.m_ptsCount = pathSize;
    params.m_fullLength = fullLength;
    params.m_pathOffset = pathOffset;
    params.m_fontDesc = fontDesc;
    params.m_logText = strings::MakeUniString(utf8Text);
    params.m_depth = depth;
    params.m_log2vis = true;
    params.m_textOffset = textOffset;
    params.m_glyphCache = glyphCache();
    params.m_pivot = path[0];

    shared_ptr<PathTextElement> pte(new PathTextElement(params));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    if (!m_overlay.get())
      pte->draw(this, id);
    else
      m_overlay->processOverlayElement(pte);
  }

  void OverlayRenderer::drawPathText(FontDesc const & fontDesc,
                                     m2::PointD const * path,
                                     size_t pathSize,
                                     string const & utf8Text,
                                     double fullLength,
                                     double pathOffset,
                                     double const * textOffsets,
                                     size_t offsSize,
                                     double depth)
  {
    if (!m_drawTexts)
      return;

    for (unsigned i = 0; i < offsSize; ++i)
      drawPathText(fontDesc,
                   path,
                   pathSize,
                   utf8Text,
                   fullLength,
                   pathOffset,
                   textOffsets[i],
                   depth);
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
