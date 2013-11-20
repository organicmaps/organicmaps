#include "drop_tile_message.hpp"

namespace df
{
  DropTileMessage::DropTileMessage(const TileKey & tileKey)
    : m_tileKey(tileKey)
  {
    SetType(DropTile);
  }

  const TileKey & DropTileMessage::GetKey() const
  {
    return m_tileKey;
  }
}
