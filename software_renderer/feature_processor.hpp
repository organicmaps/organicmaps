#pragma once

#include "software_renderer/feature_styler.hpp"
#include "software_renderer/area_info.hpp"
#include "software_renderer/path_info.hpp"

#include "drape/pointers.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/rect2d.hpp"

#include <list>
#include <vector>

class ScreenBase;

namespace software_renderer
{

struct FeatureData
{
  FeatureStyler m_styler;
  FeatureID m_id;

  std::list<PathInfo> m_pathes;
  std::list<AreaInfo> m_areas;
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

  bool operator()(FeatureType & f);

private:
  ref_ptr<CPUDrawer> m_drawer;
  m2::RectD m_rect;
  ScreenBase const & m_convertor;
  int m_zoom;
  bool m_hasAnyFeature;

  void PreProcessKeys(std::vector<drule::Key> & keys) const;
};
}  // namespace software_renderer
