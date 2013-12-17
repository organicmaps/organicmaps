#pragma once

#include "tile_info.hpp"
#include "threads_commutator.hpp"

namespace df
{
  class Message;
  class MapShape;

  class EngineContext
  {
  public:
    EngineContext(ThreadsCommutator * commutator);

    void BeginReadTile(TileKey const & key);
    /// If you call this method, you may forget about shape.
    /// It will be proccessed and delete later
    void InsertShape(TileKey const & key, MapShape const * shape);
    void EndReadTile(TileKey const & key);

  private:
    void PostMessage(Message * message);

  private:
    ThreadsCommutator * m_commutator;
  };
}
