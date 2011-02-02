#pragma once

#include "path_renderer.hpp"

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

    private:

      class TextObj
      {
        m2::PointD m_pt;
        uint8_t m_size;
        string m_utf8Text;
        bool m_isMasked;
        double m_depth;
        bool m_isFixedFont;
        bool m_log2vis;
        yg::Color m_color;
        yg::Color m_maskColor;

      public:
        TextObj(m2::PointD const & pt, string const & txt, uint8_t sz, yg::Color const & c, bool isMasked, yg::Color const & maskColor, double d, bool isFixedFont, bool log2vis)
          : m_pt(pt), m_size(sz), m_utf8Text(txt), m_isMasked(isMasked), m_depth(d), m_isFixedFont(isFixedFont), m_log2vis(log2vis), m_color(c), m_maskColor(maskColor)
        {
        }

        void Draw(TextRenderer * pTextRenderer) const;
        m2::RectD const GetLimitRect(TextRenderer * pTextRenderer) const;

        struct better_depth
        {
          bool operator() (TextObj const & r1, TextObj const & r2) const
          {
            return r1.m_depth > r2.m_depth;
          }
        };
      };

      m4::Tree<TextObj> m_tree;

      static wstring Log2Vis(wstring const & str);

      template <class ToDo>
          void ForEachGlyph(uint8_t fontSize, yg::Color const & color, wstring const & text, bool isMask, bool isFixedFont, ToDo toDo);

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     float angle,
                     float blOffset,
                     CharStyle const * p,
                     double depth);


      bool drawPathTextImpl(m2::PointD const * path,
                        size_t s,
                        uint8_t fontSize,
                        yg::Color const & color,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        TextPos pos,
                        bool isMasked,
                        double depth,
                        bool isFixedFont);

      /// Drawing text from point rotated by the angle.
      void drawTextImpl(m2::PointD const & pt,
                        float angle,
                        uint8_t fontSize,
                        yg::Color const & color,
                        string const & utf8Text,
                        bool isMasked,
                        yg::Color const & maskColor,
                        double depth,
                        bool fixedFont,
                        bool log2vis);


    public:

      typedef PathRenderer base_t;

      TextRenderer(base_t::Params const & params);

      /// Drawing text from point rotated by the angle.
      void drawText(m2::PointD const & pt,
                    float angle,
                    uint8_t fontSize,
                    yg::Color const & color,
                    string const & utf8Text,
                    bool isMasked,
                    yg::Color const & maskColor,
                    double depth,
                    bool fixedFont,
                    bool log2vis);

      m2::RectD const textRect(string const & utf8Text,
                               uint8_t fontSize,
                               bool fixedFont,
                               bool log2vis);

      /// Drawing text in the middle of the path.
      bool drawPathText(m2::PointD const * path,
                        size_t s,
                        uint8_t fontSize,
                        yg::Color const & color,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        TextPos pos,
                        bool isMasked,
                        yg::Color const & maskColor,
                        double depth,
                        bool isFixedFont = false);


      void setClipRect(m2::RectI const & rect);

      void endFrame();
    };
  }
}
