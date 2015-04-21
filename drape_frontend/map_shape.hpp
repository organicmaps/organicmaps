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
  using MapShapes = list<dp::MasterPointer<MapShape>>;

  MapShapeReadedMessage(TileKey const & key, MapShapes && shapes)
    : m_key(key), m_shapes(move(shapes))
  {}

  Type GetType() const override { return Message::MapShapeReaded; }

  ~MapShapeReadedMessage()
  {
    for (dp::MasterPointer<MapShape> & shape : m_shapes)
      shape.Destroy();

    m_shapes.clear();
  }

  TileKey const & GetKey() const { return m_key; }

  MapShapes const & GetShapes() { return m_shapes; }

private:
  TileKey m_key;
  MapShapes m_shapes;
};

} // namespace df
