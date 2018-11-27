#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/drape_measurer.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"

#include "indexer/scales.hpp"

#include "platform/preferred_languages.hpp"

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <functional>

using namespace std::placeholders;

namespace df
{
TileInfo::TileInfo(drape_ptr<EngineContext> && engineContext)
  : m_context(std::move(engineContext))
  , m_isCanceled(false)
{}

m2::RectD TileInfo::GetGlobalRect() const
{
  return GetTileKey().GetGlobalRect();
}

void TileInfo::ReadFeatureIndex(MapDataProvider const & model)
{
  if (!DoNeedReadIndex())
    return;

  CheckCanceled();

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
  }, GetGlobalRect(), GetZoomLevel());
}

void TileInfo::ReadFeatures(MapDataProvider const & model)
{
#if defined(DRAPE_MEASURER_BENCHMARK) && defined(TILES_STATISTIC)
  DrapeMeasurer::Instance().StartTileReading();
#endif
  m_context->BeginReadTile();

  // Reading can be interrupted by exception throwing
  SCOPE_GUARD(ReleaseReadTile, std::bind(&EngineContext::EndReadTile, m_context.get()));

  ReadFeatureIndex(model);
  CheckCanceled();

  m_context->GetMetalineManager()->Update(m_mwms);

  if (!m_featureInfo.empty())
  {
    std::sort(m_featureInfo.begin(), m_featureInfo.end());
    auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
    RuleDrawer drawer(std::bind(&TileInfo::InitStylist, this, deviceLang, _1, _2),
                      std::bind(&TileInfo::IsCancelled, this), model.m_isCountryLoadedByName,
                      model.GetFilter(), make_ref(m_context));
    model.ReadFeatures(std::bind<void>(std::ref(drawer), _1), m_featureInfo);
  }
#if defined(DRAPE_MEASURER_BENCHMARK) && defined(TILES_STATISTIC)
  DrapeMeasurer::Instance().EndTileReading();
#endif
}

void TileInfo::Cancel()
{
  m_isCanceled = true;
}

bool TileInfo::IsCancelled() const
{
  return m_isCanceled;
}

void TileInfo::InitStylist(int8_t deviceLang, FeatureType & f, Stylist & s)
{
  CheckCanceled();
  df::InitStylist(f, deviceLang, m_context->GetTileKey().m_zoomLevel,
                  m_context->Is3dBuildingsEnabled(), s);
}

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::CheckCanceled() const
{
  if (m_isCanceled)
    MYTHROW(ReadCanceledException, ());
}

int TileInfo::GetZoomLevel() const
{
  return ClipTileZoomByMaxDataZoom(m_context->GetTileKey().m_zoomLevel);
}
}  // namespace df
