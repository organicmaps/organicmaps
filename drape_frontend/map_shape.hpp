#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../drape/pointers.hpp"

class Batcher;

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
  };

  struct ToPoint3DFunctor
  {
    ToPoint3DFunctor(float depth)
      : m_depth(depth)
    {}

    Point3D operator ()(m2::PointF const & p)
    {
      return Point3D(p.x, p.y, m_depth);
    }

  private:
    float m_depth;
  };
  ///

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
