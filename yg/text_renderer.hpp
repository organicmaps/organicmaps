#pragma once

#include "shape_renderer.hpp"
#include "defines.hpp"
#include "font_desc.hpp"
#include "text_element.hpp"

#include "../geometry/tree4d.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class BaseTexture;

    class TextRenderer : public ShapeRenderer
    {
    public:

      class TextObj
      {
        StraightTextElement m_elem;
        mutable bool m_needRedraw;
        mutable bool m_frozen;

      public:

        TextObj(StraightTextElement const & elem);
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
      typedef map<string, list<PathTextElement> > path_text_elements;
      path_text_elements m_pathTexts;

      void checkTextRedraw();
      bool m_needTextRedraw;

      bool drawPathTextImpl(FontDesc const & fontDesc,
                            m2::PointD const * path,
                            size_t s,
                            string const & utf8Text,
                            double fullLength,
                            double pathOffset,
                            yg::EPosition pos,
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

      typedef ShapeRenderer base_t;

      struct Params : base_t::Params
      {
        bool m_textTreeAutoClean;
        bool m_useTextTree;
        bool m_drawTexts;
        bool m_doPeriodicalTextUpdate;
        Params();
      };

      TextRenderer(Params const & params);

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     ang::AngleD const & angle,
                     float blOffset,
                     CharStyle const * p,
                     double depth);

      /// Drawing text from point rotated by the angle.
      void drawText(FontDesc const & fontDesc,
                    m2::PointD const & pt,
                    yg::EPosition pos,
                    float angle,
                    string const & utf8Text,
                    double depth,
                    bool log2vis);

      /// Drawing text in the middle of the path.
      bool drawPathText(FontDesc const & fontDesc,
                        m2::PointD const * path,
                        size_t s,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        yg::EPosition pos,
                        double depth);


      void setClipRect(m2::RectI const & rect);

      void endFrame();

      void clearTextTree();
      /// shift all elements in the tree by the specified offset
      /// leaving only those elements, which intersect the specified rect
      /// boosting their priority to the top for them not to be filtered away,
      /// when the new texts arrive
      void offsetTextTree(m2::PointD const & offs, m2::RectD const & r);

      void offsetTexts(m2::PointD const & offs, m2::RectD const & r);
      void offsetPathTexts(m2::PointD const & offs, m2::RectD const & r);

      /// flush texts upon any function call.
      void setNeedTextRedraw(bool flag);

      void drawPath(m2::PointD const * points, size_t pointsCount, double offset, uint32_t styleID, double depth);

      void updateActualTarget();
    };
  }
}
