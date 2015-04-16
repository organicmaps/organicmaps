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
  virtual void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const = 0;
};

class MapShapeReadedMessage : public Message
{
public:
  using TMapShapes = list<drape_ptr<MapShape>>;

  MapShapeReadedMessage(TileKey const & key, TMapShapes && shapes)
    : m_key(key), m_shapes(move(shapes))
  {}

  Type GetType() const override { return Message::MapShapeReaded; }

  TileKey const & GetKey() const { return m_key; }

  TMapShapes const & GetShapes() { return m_shapes; }

private:
  TileKey m_key;
  TMapShapes m_shapes;
};

} // namespace df
