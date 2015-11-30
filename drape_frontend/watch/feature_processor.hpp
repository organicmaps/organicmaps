#pragma once

#include "drape_frontend/watch/feature_styler.hpp"
#include "drape_frontend/watch/area_info.hpp"
#include "drape_frontend/watch/path_info.hpp"

#include "drape/pointers.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/rect2d.hpp"

class ScreenBase;

namespace df
{
namespace watch
{

struct FeatureData
{
  FeatureStyler m_styler;
  FeatureID m_id;

  list<PathInfo> m_pathes;
  list<AreaInfo> m_areas;
  m2::PointD m_point;
};

class CPUDrawer;

class FeatureProcessor
{
public:

  FeatureProcessor(ref_ptr<CPUDrawer> drawer,
                   m2::RectD const & r,
                   ScreenBase const & convertor,
                   int zoomLevel);

  bool operator()(FeatureType const & f);

private:
  ref_ptr<CPUDrawer> m_drawer;
  m2::RectD m_rect;
  set<string> m_coasts;
  ScreenBase const & m_convertor;
  int m_zoom;
  bool m_hasNonCoast;
  bool m_hasAnyFeature;

  void PreProcessKeys(vector<drule::Key> & keys) const;
};

}
}
