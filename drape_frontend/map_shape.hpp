#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../drape/pointers.hpp"

namespace dp
{
  class Batcher;
  class TextureSetHolder;
}

namespace df
{

class MapShape
{
public:
  virtual ~MapShape(){}
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const = 0;
};

class MapShapeReadedMessage : public Message
{
public:
  MapShapeReadedMessage(TileKey const & key, dp::TransferPointer<MapShape> shape)
    : m_key(key), m_shape(shape)
  {
    SetType(MapShapeReaded);
  }

  ~MapShapeReadedMessage()
  {
    m_shape.Destroy();
  }

  TileKey const & GetKey() const { return m_key; }
  /// return non const reference for correct construct MasterPointer
  dp::TransferPointer<MapShape> & GetShape() { return m_shape; }

private:
  TileKey m_key;
  dp::TransferPointer<MapShape> m_shape;
};

} // namespace df
