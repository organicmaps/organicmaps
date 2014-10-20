#pragma once

#include "text_renderer.hpp"
#include "overlay.hpp"
#include "circle_element.hpp"
#include "symbol_element.hpp"
#include "straight_text_element.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"


namespace graphics
{
  class OverlayRenderer : public TextRenderer
  {
  private:

    bool m_drawTexts;
    bool m_drawSymbols;
    shared_ptr<graphics::OverlayStorage> m_overlayStorage;

  public:

    struct Params : public TextRenderer::Params
    {
      bool m_drawTexts;
      bool m_drawSymbols;
      Params();
    };

    OverlayRenderer(Params const & p);

    /// Drawing POI symbol
    void drawSymbol(SymbolElement::Params & params);
    void drawSymbol(m2::PointD const & pt, string const & symbolName, EPosition pos, int depth);

    /// Drawing circle
    void drawCircle(CircleElement::Params & params);
    void drawCircle(m2::PointD const & pt,
                    Circle::Info const & ci,
                    EPosition pos,
                    double depth);

    void drawCircledSymbol(SymbolElement::Params const & symParams,
                           CircleElement::Params const & circleParams);

    /// drawing straight text
    void drawText(FontDesc const & fontDesc,
                  m2::PointD const & pt,
                  graphics::EPosition pos,
                  string const & utf8Text,
                  double depth,
                  bool log2vis,
                  bool doSplit = false);

    void drawTextEx(StraightTextElement::Params & params);

    void drawTextEx(FontDesc const & primaryFont,
                    FontDesc const & secondaryFont,
                    m2::PointD const & pt,
                    graphics::EPosition pos,
                    string const & text,
                    string const & secondaryText,
                    double depth,
                    bool log2vis,
                    bool doSplit = false);

    void drawPathText(FontDesc const & fontDesc,
                      m2::PointD const * path,
                      size_t s,
                      string const & utf8Text,
                      double fullLength,
                      double pathOffset,
                      double textOffset,
                      double depth);

    /// drawing text on the path
    void drawPathText(FontDesc const & fontDesc,
                      m2::PointD const * path,
                      size_t s,
                      string const & utf8Text,
                      double fullLength,
                      double pathOffset,
                      double const * textOffsets,
                      size_t offsSize,
                      double depth);

    void setOverlay(shared_ptr<OverlayStorage> const & overlayStorage);
    void resetOverlay();
  };
}
