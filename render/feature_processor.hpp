#pragma once

#include "events.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/rect2d.hpp"

#include "graphics/glyph_cache.hpp"

class ScreenBase;

class redraw_operation_cancelled {};

namespace fwork
{
#ifndef USE_DRAPE
  class FeatureProcessor
  {
    m2::RectD m_rect;

    set<string> m_coasts;

    ScreenBase const & m_convertor;

    shared_ptr<PaintEvent> m_paintEvent;

    int m_zoom;
    bool m_hasNonCoast;
    bool m_hasAnyFeature;

    graphics::GlyphCache * m_glyphCache;

    inline Drawer * GetDrawer() const { return m_paintEvent->drawer(); }

    void PreProcessKeys(vector<drule::Key> & keys) const;

  public:

    FeatureProcessor(m2::RectD const & r,
                     ScreenBase const & convertor,
                     shared_ptr<PaintEvent> const & paintEvent,
                     int scaleLevel);

    bool operator() (FeatureType const & f);

    bool IsEmptyDrawing() const;
  };
#endif //USE_DRAPE
}
