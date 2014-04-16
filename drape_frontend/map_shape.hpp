#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../drape/pointers.hpp"

class Batcher;
class TextureSetHolder;

namespace df
{

/// Utils
struct Point3D
{
  float m_x;
  float m_y;
  float m_z;

  Point3D(float x, float y, float z)
    : m_x(x)
    , m_y(y)
    , m_z(z)
  {}

  static Point3D From2D(m2::PointF const & src, float thirdComponent = 0)
  {
    return Point3D(src.x, src.y, thirdComponent);
  }
};
///

class MapShape
{
public:
  virtual ~MapShape(){}
  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const = 0;
};

class MapShapeReadedMessage : public Message
{
public:
  MapShapeReadedMessage(TileKey const & key, TransferPointer<MapShape> shape)
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
  TransferPointer<MapShape> & GetShape() { return m_shape; }

private:
  TileKey m_key;
  TransferPointer<MapShape> m_shape;
};

} // namespace df
