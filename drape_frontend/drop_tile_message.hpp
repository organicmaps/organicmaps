#pragma once

#include "message.hpp"
#include "tile_info.hpp"

namespace df
{
  class DropTileMessage : public Message
  {
  public:
    DropTileMessage(const TileKey & tileKey);

    const TileKey & GetKey() const;

  private:
    TileKey m_tileKey;
  };
}
