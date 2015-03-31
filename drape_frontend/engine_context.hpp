#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"

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
  /// If you call this method, you may forget about shape.
  /// It will be proccessed and delete later
  void InsertShape(dp::TransferPointer<MapShape> shape);
  void EndReadTile();

private:
  void PostMessage(dp::TransferPointer<Message> message);

private:
  TileKey m_tileKey;
  dp::RefPointer<ThreadsCommutator> m_commutator;
};

} // namespace df
