#pragma once

#include "text_renderer.hpp"
#include "../geometry/tree4d.hpp"
#include "../std/map.hpp"

namespace yg
{
  namespace gl
  {
    class SymbolRenderer : public TextRenderer
    {
    private:

      struct SymbolObject
      {
        m2::PointD m_pt;
        EPosition  m_pos;
        uint32_t   m_styleID;
        double     m_depth;

        SymbolObject(m2::PointD const & pt, uint32_t styleID, EPosition pos, double depth);

        m2::RectD const GetLimitRect(SymbolRenderer * p) const;
        void Draw(SymbolRenderer * p) const;
      };

      typedef map<uint32_t, m4::Tree<SymbolObject> > symbols_map_t;
      symbols_map_t m_symbolsMap;

      m2::PointD const getPosPt(m2::PointD const & pt, m2::RectD const & texRect, EPosition pos);

      void drawSymbolImpl(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth);

    public:

      typedef TextRenderer base_t;

      SymbolRenderer(base_t::Params const & params);

      void drawSymbol(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth);
      void drawCircle(m2::PointD const & pt, uint32_t styleID, EPosition pos, int depth);

      void setClipRect(m2::RectI const & rect);
      void endFrame();
    };
  }
}
