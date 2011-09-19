#pragma once

#include "defines.hpp"

#include "../base/string_utils.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/aa_rect2d.hpp"
#include "../geometry/angles.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/shared_ptr.hpp"

#include "glyph_cache.hpp"

namespace yg
{
  class GlyphCache;
  struct FontDesc;

  struct GlyphLayoutElem
  {
    wchar_t m_sym;
    GlyphMetrics m_metrics;
    ang::AngleD m_angle;
    m2::PointD m_pt;

    void transform(math::Matrix<double, 3, 3> const & m);
  };

  class GlyphLayout
  {
  private:

    size_t m_firstVisible;
    size_t m_lastVisible;

    vector<GlyphLayoutElem> m_entries;

    m2::AARectD m_limitRect;

    m2::PointD m_pivot;

    double getKerning(GlyphLayoutElem const & prevElem, GlyphLayoutElem const & curElem);
    void computeMinLimitRect();
    void recalcPivot();

  public:

    GlyphLayout();

    GlyphLayout(GlyphLayout const & layout, math::Matrix<double, 3, 3> const & m);

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const & pt,
                strings::UniString const & visText,
                yg::EPosition pos);

    GlyphLayout(GlyphCache * glyphCache,
                FontDesc const & font,
                m2::PointD const * pts,
                size_t ptsCount,
                strings::UniString const & visText,
                double fullLength,
                double pathOffset,
                yg::EPosition pos);

    size_t firstVisible() const;
    size_t lastVisible() const;

    vector<GlyphLayoutElem> const & entries() const;

    m2::AARectD const limitRect() const;

    m2::PointD const & pivot() const;

    void setPivot(m2::PointD const & pv);
  };
}
