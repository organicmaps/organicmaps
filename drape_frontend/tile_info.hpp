#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/memory_feature_index.hpp"

#include "indexer/feature_decl.hpp"

#include "base/mutex.hpp"
#include "base/exception.hpp"

#include "std/vector.hpp"
#include "std/noncopyable.hpp"

class FeatureType;

namespace df
{

class MapDataProvider;
class EngineContext;
class Stylist;

class TileInfo : private noncopyable
{
public:
  DECLARE_EXCEPTION(ReadCanceledException, RootException);

  TileInfo(TileKey const & key);

  void ReadFeatureIndex(MapDataProvider const & model);
  void ReadFeatures(MapDataProvider const & model,
                    MemoryFeatureIndex & memIndex,
                    EngineContext & context);
  void Cancel(MemoryFeatureIndex & memIndex);

  m2::RectD GetGlobalRect() const;
  TileKey const & GetTileKey() const { return m_key; }

  bool operator <(TileInfo const & other) const { return m_key < other.m_key; }

private:
  void ProcessID(FeatureID const & id);
  void InitStylist(FeatureType const & f, Stylist & s);
  void RequestFeatures(MemoryFeatureIndex & memIndex, vector<size_t> & featureIndexes);
  void CheckCanceled() const;
  bool DoNeedReadIndex() const;

  int GetZoomLevel() const;

private:
  TileKey m_key;
  vector<FeatureInfo> m_featureInfo;

  bool m_isCanceled;
  threads::Mutex m_mutex;
};

} // namespace df
