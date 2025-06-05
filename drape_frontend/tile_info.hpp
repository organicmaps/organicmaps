#pragma once

#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/feature_decl.hpp"

#include "base/exception.hpp"
#include "base/macros.hpp"

#include <atomic>
#include <set>
#include <vector>

class FeatureType;

namespace df
{
class MapDataProvider;
class Stylist;

class TileInfo
{
public:
  DECLARE_EXCEPTION(ReadCanceledException, RootException);

  TileInfo(drape_ptr<EngineContext> && engineContext);

  void ReadFeatures(MapDataProvider const & model);
  void Cancel();
  bool IsCancelled() const;

  m2::RectD GetGlobalRect() const;
  TileKey const & GetTileKey() const { return m_context->GetTileKey(); }
  bool operator<(TileInfo const & other) const { return GetTileKey() < other.GetTileKey(); }

private:
  void ReadFeatureIndex(MapDataProvider const & model);
  void ThrowIfCancelled() const;
  bool DoNeedReadIndex() const;

  int GetZoomLevel() const;

private:
  drape_ptr<EngineContext> m_context;
  std::vector<FeatureID> m_featureInfo;
  std::atomic<bool> m_isCanceled;
  std::set<MwmSet::MwmId> m_mwms;

  DISALLOW_COPY_AND_MOVE(TileInfo);
};
}  // namespace df
