#pragma once

#include "text_renderer.hpp"
#include "../std/shared_ptr.hpp"
#include "info_layer.hpp"

namespace yg
{
  namespace gl
  {
    class OverlayRenderer : public TextRenderer
    {
    private:

      bool m_drawTexts;
      bool m_drawSymbols;
      shared_ptr<yg::InfoLayer> m_infoLayer;

      typedef map<m2::PointI, shared_ptr<OverlayElement> > TElements;

      TElements m_elements;

    public:

      struct Params : public TextRenderer::Params
      {
        bool m_drawTexts;
        bool m_drawSymbols;
        shared_ptr<yg::InfoLayer> m_infoLayer;
        Params();
      };

      OverlayRenderer(Params const & p);

      /// Drawing POI symbol
      void drawSymbol(m2::PointD const & pt, string const & symbolName, EPosition pos, int depth);

      /// Drawing circle
      void drawCircle(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth);

      /// drawing straight text
      void drawText(FontDesc const & fontDesc,
                    m2::PointD const & pt,
                    yg::EPosition pos,
                    string const & utf8Text,
                    double depth,
                    bool log2vis);

      /// drawing text on the path
      bool drawPathText(FontDesc const & fontDesc,
                        m2::PointD const * path,
                        size_t s,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        yg::EPosition pos,
                        double depth);

      void setInfoLayer(shared_ptr<InfoLayer> const & infoLayer);

      shared_ptr<InfoLayer> const & infoLayer() const;

      void resetInfoLayer();

      void endFrame();
    };
  }
}
