#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"

namespace df
{

class Message;

class EngineContext
{
public:
  EngineContext(TileKey tileKey, ref_ptr<ThreadsCommutator> commutator);

  TileKey const & GetTileKey() const { return m_tileKey; }

  void BeginReadTile();
  /// If you call this method, you may forget about shape.
  /// It will be proccessed and delete later
  void InsertShape(drape_ptr<MapShape> && shape);
  void Flush();
  void EndReadTile();

private:
  void PostMessage(drape_ptr<Message> && message);

private:
  TileKey m_tileKey;
  ref_ptr<ThreadsCommutator> m_commutator;
  list<drape_ptr<MapShape>> m_mapShapes;
};

} // namespace df
