#pragma once

#include "graphics/glyph_cache.hpp"
#include "graphics/defines.hpp"
#include "graphics/text_path.hpp"
#include "graphics/font_desc.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/any_rect2d.hpp"
#include "geometry/angles.hpp"

#include "base/string_utils.hpp"


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
  protected:
    size_t m_firstVisible;
    size_t m_lastVisible;

    graphics::FontDesc m_fontDesc;

    buffer_vector<GlyphMetrics, 8> m_metrics;
    buffer_vector<GlyphLayoutElem, 8> m_entries;
    buffer_vector<m2::AnyRectD, 1> m_boundRects;

    m2::PointD m_pivot;
    m2::PointD m_offset;

    double m_textLength;

    void computeBoundRects();

    void addGlyph(GlyphCache * glyphCache,
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

  protected:
    GlyphLayout(FontDesc const & font);

  public:
    GlyphLayout() {}

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const & pt,
                strings::UniString const & visText,
                graphics::EPosition pos,
                unsigned maxWidth = numeric_limits<unsigned>::max());

    size_t firstVisible() const;
    size_t lastVisible() const;

    buffer_vector<GlyphLayoutElem, 8> const & entries() const { return m_entries; }
    buffer_vector<m2::AnyRectD, 1> const & boundRects() const { return m_boundRects; }

    /// @note! Used only in StraightTextElement.
    m2::RectD GetLastGlobalRect() const { return m_boundRects.back().GetGlobalRect(); }

    graphics::FontDesc const & fontDesc() const;

    m2::PointD const & pivot() const;
    void setPivot(m2::PointD const & pv);

    m2::PointD const & offset() const;
    void setOffset(m2::PointD const & offs);

    int baseLineOffset();
  };

  class GlyphLayoutPath : public GlyphLayout
  {
    TextPath m_path;
    strings::UniString m_visText;

    double m_textOffset;

    void recalcAlongPath();

  public:
    GlyphLayoutPath() {}

    GlyphLayoutPath(GlyphLayoutPath const & layout,
                math::Matrix<double, 3, 3> const & m);

    GlyphLayoutPath(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const * pts,
                size_t ptsCount,
                strings::UniString const & visText,
                double fullLength,
                double pathOffset,
                double textOffset);

    bool IsFullVisible() const;
  };
}
