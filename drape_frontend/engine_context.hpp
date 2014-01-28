#pragma once

#include "threads_commutator.hpp"

#include "../drape/pointers.hpp"

#include "../map/scales_processor.hpp"

namespace df
{
  class Message;
  class MapShape;
  struct TileKey;

  class EngineContext
  {
  public:
    EngineContext(RefPointer<ThreadsCommutator> commutator, ScalesProcessor const & processor);

    ScalesProcessor const & GetScalesProcessor() const;

    void BeginReadTile(TileKey const & key);
    /// If you call this method, you may forget about shape.
    /// It will be proccessed and delete later
    void InsertShape(TileKey const & key, TransferPointer<MapShape> shape);
    void EndReadTile(TileKey const & key);

  private:
    void PostMessage(Message * message);

  private:
    RefPointer<ThreadsCommutator> m_commutator;
    ScalesProcessor m_scalesProcessor;
  };
}
