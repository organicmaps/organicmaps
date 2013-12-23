#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../drape/pointers.hpp"

class Batcher;

namespace df
{
  class MapShape
  {
  public:
    virtual ~MapShape(){}
    virtual void Draw(RefPointer<Batcher> batcher) const = 0;
  };

  class MapShapeReadedMessage : public Message
  {
  public:
    MapShapeReadedMessage(const TileKey & key, TransferPointer<MapShape> shape)
      : m_key(key), m_shape(shape)
    {
      SetType(MapShapeReaded);
    }

    ~MapShapeReadedMessage()
    {
      m_shape.Destroy();
    }

    const TileKey & GetKey() const { return m_key; }
    /// return non const reference for correct construct MasterPointer
    TransferPointer<MapShape> & GetShape() { return m_shape; }

  private:
    TileKey m_key;
    TransferPointer<MapShape> m_shape;
  };
}
