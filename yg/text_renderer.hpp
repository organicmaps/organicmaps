#pragma once

#include "path_renderer.hpp"
#include "defines.hpp"
#include "font_desc.hpp"

#include "../geometry/tree4d.hpp"

#include "../std/shared_ptr.hpp"


namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class BaseTexture;

    class TextRenderer : public PathRenderer
    {
    public:

      enum TextPos { under_line, middle_line, above_line };


      class TextObj
      {
        FontDesc m_fontDesc;
        m2::PointD m_pt;
        yg::EPosition m_pos;
        string m_utf8Text;
        double m_depth;
        mutable bool m_needRedraw;
        mutable bool m_frozen;
        bool m_log2vis;

      public:

        TextObj(FontDesc const & fontDesc,
                m2::PointD const & pt,
                yg::EPosition pos,
                string const & txt,
                double depth,
                bool log2vis);
        void Draw(TextRenderer * pTextRenderer) const;
        m2::RectD const GetLimitRect(TextRenderer * pTextRenderer) const;
        void SetNeedRedraw(bool needRedraw) const;
        bool IsNeedRedraw() const;
        bool IsFrozen() const;
        void Offset(m2::PointD const & pt);
        string const & Text() const;

        static bool better_text(TextObj const & r1, TextObj const & r2);
      };

    private:

      m4::Tree<TextObj> m_tree;

      void checkTextRedraw();
      bool m_needTextRedraw;

      static wstring Log2Vis(wstring const & str);

      template <class ToDo>
          void ForEachGlyph(FontDesc const & fontDesc, wstring const & text, ToDo toDo);

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     float angle,
                     float blOffset,
                     CharStyle const * p,
                     double depth);


      bool drawPathTextImpl(FontDesc const & fontDesc,
                            m2::PointD const * path,
                            size_t s,
                            string const & utf8Text,
                            double fullLength,
                            double pathOffset,
                            TextPos pos,
                            double depth);

      /// Drawing text from point rotated by the angle.
      void drawTextImpl(FontDesc const & fontDesc,
                        m2::PointD const & pt,
                        yg::EPosition pos,
                        float angle,
                        string const & utf8Text,
                        double depth,
                        bool log2vis);

      bool m_textTreeAutoClean;
      bool m_useTextTree;
      bool m_drawTexts;
      bool m_doPeriodicalTextUpdate;

    public:

      typedef PathRenderer base_t;

      struct Params : base_t::Params
      {
        bool m_textTreeAutoClean;
        bool m_useTextTree;
        bool m_drawTexts;
        bool m_doPeriodicalTextUpdate;
        Params();
      };

      TextRenderer(Params const & params);

      /// Drawing text from point rotated by the angle.
      void drawText(FontDesc const & fontDesc,
                    m2::PointD const & pt,
                    yg::EPosition pos,
                    float angle,
                    string const & utf8Text,
                    double depth,
                    bool log2vis);

      m2::RectD const textRect(FontDesc const & fontDesc,
                               string const & utf8Text,
                               bool log2vis);

      /// Drawing text in the middle of the path.
      bool drawPathText(FontDesc const & fontDesc,
                        m2::PointD const * path,
                        size_t s,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        TextPos pos,
                        double depth);


      void setClipRect(m2::RectI const & rect);

      void endFrame();

      void clearTextTree();
      /// shift all elements in the tree by the specified offset
      /// leaving only those elements, which intersect the specified rect
      /// boosting their priority to the top for them not to be filtered away,
      /// when the new texts arrive
      void offsetTextTree(m2::PointD const & offs, m2::RectD const & r);

      /// flush texts upon any function call.
      void setNeedTextRedraw(bool flag);

      void drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth);

      void updateActualTarget();
    };
  }
}
