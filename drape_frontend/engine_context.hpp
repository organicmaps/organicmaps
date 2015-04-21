#pragma once

#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

namespace df
{

class MapShape;
class Message;

class EngineContext
{
public:
  EngineContext(TileKey tileKey, dp::RefPointer<ThreadsCommutator> commutator);

  TileKey const & GetTileKey() const { return m_tileKey; }

  void BeginReadTile();
  void BeginReadFeature(FeatureID const & featureId);
  /// If you call this method, you may forget about shape.
  /// It will be proccessed and delete later
  void InsertShape(FeatureID const & featureId, dp::TransferPointer<MapShape> shape);
  void EndReadFeature(FeatureID const & featureId);
  void EndReadTile();

private:
  void PostMessage(dp::TransferPointer<Message> message);

private:
  TileKey m_tileKey;
  dp::RefPointer<ThreadsCommutator> m_commutator;

  using MapShapeStorage = map<FeatureID, list<dp::MasterPointer<MapShape>>>;
  MapShapeStorage m_mapShapeStorage;
};

class EngineContextReadFeatureGuard
{
public:
  EngineContextReadFeatureGuard(EngineContext & context, FeatureID const & featureId)
    : m_context(context), m_id(featureId)
  {
    m_context.BeginReadFeature(m_id);
  }

  ~EngineContextReadFeatureGuard()
  {
    m_context.EndReadFeature(m_id);
  }

private:
  EngineContext & m_context;
  FeatureID m_id;
};

} // namespace df
