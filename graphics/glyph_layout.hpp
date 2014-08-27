#pragma once

#include "glyph_cache.hpp"
#include "defines.hpp"
#include "text_path.hpp"
#include "font_desc.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../geometry/angles.hpp"

#include "../base/string_utils.hpp"


namespace graphics
{
  class GlyphCache;

  struct GlyphLayoutElem
  {
    strings::UniChar m_sym;
    ang::AngleD m_angle;
    m2::PointD m_pt;

    void transform(math::Matrix<double, 3, 3> const & m);
  };

  class GlyphLayout
  {
  private:

    size_t m_firstVisible;
    size_t m_lastVisible;

    TextPath m_path;

    strings::UniString m_visText;

    graphics::FontDesc m_fontDesc;

    buffer_vector<GlyphMetrics, 32> m_metrics;
    buffer_vector<GlyphLayoutElem, 32> m_entries;
    buffer_vector<m2::AnyRectD, 16> m_boundRects;

    m2::PointD m_pivot;
    m2::PointD m_offset;

    double m_textLength;
    double m_textOffset;

    void computeBoundRects();

    void recalcPivot();
    void recalcAlongPath();

    inline void addGlyph(GlyphCache * glyphCache,
                        GlyphKey const & key,
                        bool isFirst,
                        strings::UniChar symbol,
                        m2::RectD & boundRect,
                        m2::PointD & curPt);

    void initStraigthText(GlyphCache * glyphCache,
                          FontDesc const & font,
                          m2::PointD const & pt,
                          strings::UniString const & visText,
                          graphics::EPosition pos,
                          unsigned maxWidth);

  public:

    GlyphLayout();

    GlyphLayout(GlyphLayout const & layout,
                math::Matrix<double, 3, 3> const & m);

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const & pt,
                strings::UniString const & visText,
                graphics::EPosition pos);

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const & pt,
                strings::UniString const & visText,
                graphics::EPosition pos,
                unsigned maxWidth);

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const * pts,
                size_t ptsCount,
                strings::UniString const & visText,
                double fullLength,
                double pathOffset,
                double textOffset);

    size_t firstVisible() const;
    size_t lastVisible() const;

    buffer_vector<GlyphLayoutElem, 32> const & entries() const;
    buffer_vector<m2::AnyRectD, 16> const & boundRects() const;

    graphics::FontDesc const & fontDesc() const;

    m2::PointD const & pivot() const;
    void setPivot(m2::PointD const & pv);

    m2::PointD const & offset() const;
    void setOffset(m2::PointD const & offs);

    int baseLineOffset();
  };
}
