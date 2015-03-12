#pragma once

#include "drape_frontend/message.hpp"
#include "drape_frontend/tile_info.hpp"

#include "drape/pointers.hpp"

namespace dp
{
  class Batcher;
  class TextureManager;
}

namespace df
{

class MapShape
{
public:
  virtual ~MapShape(){}
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> textures) const = 0;
};

class MapShapeReadedMessage : public Message
{
public:
  MapShapeReadedMessage(TileKey const & key, dp::TransferPointer<MapShape> shape)
    : m_key(key), m_shape(shape)
  {}

  Type GetType() const override { return Message::MapShapeReaded; }

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
