#pragma once

#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/memory_feature_index.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/feature_decl.hpp"

#include "base/exception.hpp"
#include "base/mutex.hpp"

#include "std/atomic.hpp"
#include "std/mutex.hpp"
#include "std/noncopyable.hpp"
#include "std/vector.hpp"

class FeatureType;

namespace df
{

class MapDataProvider;
class Stylist;

class TileInfo : private noncopyable
{
public:
  DECLARE_EXCEPTION(ReadCanceledException, RootException);

  TileInfo(EngineContext const & context);

  void ReadFeatureIndex(MapDataProvider const & model);
  void ReadFeatures(MapDataProvider const & model,
                    MemoryFeatureIndex & memIndex);
  void Cancel(MemoryFeatureIndex & memIndex);

  m2::RectD GetGlobalRect() const;
  TileKey const & GetTileKey() const { return m_context.GetTileKey(); }
  bool operator <(TileInfo const & other) const { return GetTileKey() < other.GetTileKey(); }

private:
  void ProcessID(FeatureID const & id);
  void InitStylist(FeatureType const & f, Stylist & s);
  void RequestFeatures(MemoryFeatureIndex & memIndex, vector<size_t> & featureIndexes);
  void CheckCanceled() const;
  bool DoNeedReadIndex() const;

  int GetZoomLevel() const;

private:
  EngineContext m_context;
  vector<FeatureInfo> m_featureInfo;

  atomic<bool> m_isCanceled;
  mutex m_mutex;
};

} // namespace df
