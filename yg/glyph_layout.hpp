#pragma once

#include "defines.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/aa_rect2d.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/shared_ptr.hpp"

#include "../geometry/point2d.hpp"
#include "glyph_cache.hpp"


namespace yg
{
  class ResourceManager;
  class Skin;
  struct FontDesc;

  struct GlyphLayoutElem
  {
    wchar_t m_sym;
    GlyphMetrics m_metrics;
    double m_angle;
    m2::PointD m_pt;
  };

  class GlyphLayout
  {
  private:

    shared_ptr<ResourceManager> m_resourceManager;

    size_t m_firstVisible;
    size_t m_lastVisible;

    vector<GlyphLayoutElem> m_entries;

    m2::AARectD m_limitRect;

    double getKerning(GlyphLayoutElem const & prevElem, GlyphLayoutElem const & curElem);
    void computeMinLimitRect();

  public:

    GlyphLayout(GlyphLayout const & layout, double shift);

    GlyphLayout(shared_ptr<ResourceManager> const & resourceManager,
                shared_ptr<Skin> const & skin,
                FontDesc const & font,
                m2::PointD const & pt,
                wstring const & text,
                yg::EPosition pos);

    GlyphLayout(shared_ptr<ResourceManager> const & resourceManager,
                FontDesc const & font,
                m2::PointD const * pts,
                size_t ptsCount,
                wstring const & text,
                double fullLength,
                double pathOffset,
                yg::EPosition pos);

    size_t firstVisible() const;
    size_t lastVisible() const;

    vector<GlyphLayoutElem> const & entries() const;

    m2::AARectD const limitRect() const;

    void offset(m2::PointD const & offs);
  };
}
