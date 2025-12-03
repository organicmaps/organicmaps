#pragma once
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include <functional>

namespace osm
{
class Editor;
}
namespace df
{
class MapShape;

/// Holds everything that calculated (initialized) _once_ for the tile processing.
struct ApplyFeatureParams
{
  std::function<void(drape_ptr<MapShape> && shape)> m_insertShape;
  TileKey m_tileKey;
  m2::RectD m_tileRect;

  VisualParams const & m_vparams;
  osm::Editor const & m_editor;

  double m_currentScaleGtoP, m_trafficScalePtoG;
  double m_minSegmentSqrLength;

  ApplyFeatureParams();
  void Init(TileKey const & tileKey);

  bool IsSimplifyLines() const { return (m_tileKey.m_zoomLevel >= 10 && m_tileKey.m_zoomLevel <= 12); }
  bool IsRelationRoutes() const { return m_tileKey.m_zoomLevel >= 12; }

  /// @return (created, obsolete)
  std::pair<bool, bool> GetEditStatus(FeatureID const & fid) const;
};

}  // namespace df
