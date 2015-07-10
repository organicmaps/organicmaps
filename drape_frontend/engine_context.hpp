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
  void Flush(list<drape_ptr<MapShape>> && shapes);
  void EndReadTile();

private:
  void PostMessage(drape_ptr<Message> && message);

  TileKey m_tileKey;
  ref_ptr<ThreadsCommutator> m_commutator;
};

} // namespace df
