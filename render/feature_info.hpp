#pragma once

#include "feature_styler.hpp"
#include "path_info.hpp"
#include "area_info.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include "std/list.hpp"


namespace graphics { class GlyphCache; }
class FeatureType;
class ScreenBase;

namespace di
{
  struct FeatureInfo
  {
    FeatureStyler m_styler;
    FeatureID m_id;

    list<di::PathInfo> m_pathes;
    list<di::AreaInfo> m_areas;
    m2::PointD m_point;

    FeatureInfo(FeatureType const & f,
                int const zoom,
                double const visualScale,
                graphics::GlyphCache * glyphCache,
                ScreenBase const * convertor,
                m2::RectD const * rect);
  };
}
