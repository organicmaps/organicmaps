#include "feature_info.hpp"

#include "indexer/feature.hpp"


namespace di
{
  FeatureInfo::FeatureInfo(FeatureType const & f,
                           int const zoom,
                           double const visualScale,
                           graphics::GlyphCache * glyphCache,
                           ScreenBase const * convertor,
                           m2::RectD const * rect)
    : m_styler(f, zoom, visualScale, glyphCache, convertor, rect),
      m_id(f.GetID())
  {}
}
