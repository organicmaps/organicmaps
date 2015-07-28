#pragma once

#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/memory_feature_index.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/feature_decl.hpp"

#include "base/exception.hpp"

#include "std/atomic.hpp"
#include "std/mutex.hpp"
#include "std/vector.hpp"

class FeatureType;

namespace dp
{
  class TextureManager;
}

namespace df
{

class MapDataProvider;
class Stylist;

class TileInfo : private noncopyable
{
public:
  DECLARE_EXCEPTION(ReadCanceledException, RootException);

  TileInfo(drape_ptr<EngineContext> && context);

  void ReadFeatures(MapDataProvider const & model,
                    MemoryFeatureIndex & memIndex,
                    ref_ptr<dp::TextureManager> texMng);
  void Cancel(MemoryFeatureIndex & memIndex);

  m2::RectD GetGlobalRect() const;
  TileKey const & GetTileKey() const { return m_context->GetTileKey(); }
  bool operator <(TileInfo const & other) const { return GetTileKey() < other.GetTileKey(); }

private:
  void ReadFeatureIndex(MapDataProvider const & model);
  void ProcessID(FeatureID const & id);
  void InitStylist(FeatureType const & f, Stylist & s);
  void RequestFeatures(MemoryFeatureIndex & memIndex, vector<FeatureID> & featuresToRead);
  void CheckCanceled() const;
  bool DoNeedReadIndex() const;

  int GetZoomLevel() const;

private:
  drape_ptr<EngineContext> m_context;
  TFeaturesInfo m_featureInfo;

  atomic<bool> m_isCanceled;
  mutex m_mutex;
};

} // namespace df
