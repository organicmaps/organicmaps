#pragma once

#include "memory_feature_index.hpp"

#include "../indexer/feature_decl.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/mutex.hpp"
#include "../base/exception.hpp"

#include "../std/vector.hpp"
#include "../std/noncopyable.hpp"


namespace model { class FeaturesFetcher; }

namespace df
{
class EngineContext;

struct TileKey
{
public:
  TileKey() : m_x(-1), m_y(-1), m_zoomLevel(-1) {}
  TileKey(int x, int y, int zoomLevel)
    : m_x(x), m_y(y), m_zoomLevel(zoomLevel) {}

  bool operator < (const TileKey & other) const
  {
    if (m_zoomLevel != other.m_zoomLevel)
      return m_zoomLevel < other.m_zoomLevel;
    if (m_y != other.m_y)
      return m_y < other.m_y;

    return m_x < other.m_x;
  }

  bool operator == (const TileKey & other) const
  {
    return m_x == other.m_x &&
        m_y == other.m_y &&
        m_zoomLevel == other.m_zoomLevel;
  }

  int m_x;
  int m_y;
  int m_zoomLevel;
};

class TileInfo : private noncopyable
{
public:

  DECLARE_EXCEPTION(ReadCanceledException, RootException);

  TileInfo(TileKey const & key, model::FeaturesFetcher & model, MemoryFeatureIndex & memIndex);

  void ReadFeatureIndex();
  void ReadFeatures(EngineContext & context);

  m2::RectD GetGlobalRect() const;

  TileKey const & GetTileKey() const { return m_key; }
  void Cancel();

  void operator ()(const FeatureID & id);
  bool operator < (const TileInfo & other) const { return m_key < other.m_key; }

private:
  void RequestFeatures(vector<size_t> & featureIndexes);
  void CheckCanceled();
  bool DoNeedReadIndex() const;

private:
  TileKey m_key;

  vector<FeatureInfo> m_featureInfo;
  model::FeaturesFetcher & m_model;
  MemoryFeatureIndex & m_memIndex;

  bool m_isCanceled;
  threads::Mutex m_mutex;
};

}
