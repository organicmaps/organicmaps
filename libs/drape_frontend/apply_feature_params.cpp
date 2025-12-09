#include "drape_frontend/apply_feature_params.hpp"

#include "editor/osm_editor.hpp"

namespace df
{
ApplyFeatureParams::ApplyFeatureParams() : m_vparams(VisualParams::Instance()), m_editor(osm::Editor::Instance()) {}

void ApplyFeatureParams::Init(TileKey const & tileKey)
{
  m_tileKey = tileKey;
  m_tileRect = tileKey.GetGlobalRect(true /* clipByDataMaxZoom */);

  int const tileSize = m_vparams.GetTileSize();
  m2::AnyRectD const rect(tileKey.GetGlobalRect(false /* clipByDataMaxZoom */));
  ScreenBase geometryConvertor;
  geometryConvertor.OnSize(0, 0, tileSize, tileSize);
  geometryConvertor.SetFromRect(rect);
  m_currentScaleGtoP = 1.0 / geometryConvertor.GetScale();

  // Here we support only two virtual tile size: 2048 px for high resolution and 1024 px for others.
  // It helps to render traffic the same on wide range of devices.
  uint32_t const trafficTileSize = m_vparams.GetVisualScale() < df::VisualParams::kXxhdpiScale ? 1024 : 2048;
  geometryConvertor.OnSize(0, 0, trafficTileSize, trafficTileSize);
  geometryConvertor.SetFromRect(rect);
  m_trafficScalePtoG = geometryConvertor.GetScale();

  m_minSegmentSqrLength = math::Pow2(4.0 * m_vparams.GetVisualScale() / m_currentScaleGtoP);
}

std::pair<bool, bool> ApplyFeatureParams::GetEditStatus(FeatureID const & fid) const
{
  auto const status = m_editor.GetFeatureStatus(fid);
  return {status == FeatureStatus::Created, status == FeatureStatus::Obsolete};
}

}  // namespace df
