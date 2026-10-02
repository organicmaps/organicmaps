#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "base/scope_guard.hpp"

#include <algorithm>

namespace df
{
TileInfo::TileInfo(drape_ptr<EngineContext> && engineContext) : m_context(std::move(engineContext)) {}

void TileInfo::ReadFeatureIndex(MapDataProvider const & model)
{
  if (!DoNeedReadIndex())
    return;

  size_t const kAverageFeaturesCount = 256;
  m_featureInfo.reserve(kAverageFeaturesCount);

  MwmSet::MwmId lastMwm;
  model.ReadFeaturesID([this, &lastMwm](FeatureID const & id)
  {
    if (m_mwms.empty() || lastMwm != id.m_mwmId)
    {
      auto result = m_mwms.insert(id.m_mwmId);
      VERIFY(result.second, ());
      lastMwm = id.m_mwmId;
    }
    m_featureInfo.push_back(id);
  }, GetTileKey().GetWrappedDataRect(), GetZoomLevel());
}

void TileInfo::ReadFeatures(MapDataProvider const & model)
{
#if defined(DRAPE_MEASURER_BENCHMARK) && defined(TILES_STATISTIC)
  DrapeMeasurer::Instance().StartTileReading();
#endif
  m_context->BeginReadTile();

  // Pair every start with an end, including cancellation and exceptions.
  SCOPE_GUARD(ReleaseReadTile, [this] { m_context->EndReadTile(); });

  if (IsCancelled())
    return;

  ReadFeatureIndex(model);
  if (IsCancelled())
    return;

  m_context->GetMetalineManager()->Update(m_mwms);

  if (!m_featureInfo.empty())
  {
    std::sort(m_featureInfo.begin(), m_featureInfo.end());

    RuleDrawer drawer(model.m_isCountryLoadedByName, make_ref(m_context), m_context->GetMapLangIndex());
    model.ReadFeatures([&drawer](FeatureType & ft) { drawer(ft); }, m_featureInfo);
#ifdef DRAW_TILE_NET
    drawer.DrawTileNet();
#endif
  }
#if defined(DRAPE_MEASURER_BENCHMARK) && defined(TILES_STATISTIC)
  DrapeMeasurer::Instance().EndTileReading();
#endif
}

void TileInfo::Cancel()
{
  m_context->Cancel();
}

bool TileInfo::IsCancelled() const
{
  return m_context->IsCancelled();
}

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

int TileInfo::GetZoomLevel() const
{
  return ClipTileZoomByMaxDataZoom(m_context->GetTileKey().m_zoomLevel);
}
}  // namespace df
