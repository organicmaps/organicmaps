#pragma once

#include "message.hpp"
#include "tile_info.hpp"

class Batcher;

namespace df
{
  class MapShape
  {
  public:
    virtual ~MapShape(){}
    virtual void Draw(Batcher * batcher) const = 0;
  };

  class MapShapeReadedMessage : public Message
  {
  public:
    MapShapeReadedMessage(const TileKey & key, MapShape const * shape)
      : m_key(key), m_shape(shape)
    {
      SetType(MapShapeReaded);
    }

    ~MapShapeReadedMessage()
    {
      delete m_shape;
    }

    const TileKey & GetKey() const { return m_key; }
    MapShape const * GetShape() const { return m_shape; }

  private:
    TileKey m_key;
    MapShape const * m_shape;
  };
}
