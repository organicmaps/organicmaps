#pragma once

#include "geometry_batcher.hpp"

#include "../geometry/tree4d.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class TextRenderer : public GeometryBatcher
    {
      class TextObj
      {
        m2::PointD m_pt;
        uint8_t m_size;
        string m_utf8Text;
        double m_depth;

      public:
        TextObj(m2::PointD const & pt, string const & txt, uint8_t sz, double d)
          : m_pt(pt), m_size(sz), m_utf8Text(txt), m_depth(d)
        {
        }

        void Draw(GeometryBatcher * pBatcher) const;
        m2::RectD GetLimitRect(GeometryBatcher * pBatcher) const;

        struct better_depth
        {
          bool operator() (TextObj const & r1, TextObj const & r2) const
          {
            return r1.m_depth > r2.m_depth;
          }
        };
      };

      m4::Tree<TextObj> m_tree;

    public:

      typedef GeometryBatcher::Params Params;

      TextRenderer(Params const & params) : GeometryBatcher(params)
      {}

      void drawText(m2::PointD const & pt,
        float angle,
        uint8_t fontSize,
        string const & utf8Text,
        double depth);

      void endFrame();
    };
  }
}
