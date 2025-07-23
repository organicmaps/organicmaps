#pragma once

#include "drape_frontend/message.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace dp
{
class Batcher;
class TextureManager;
}  // namespace dp

namespace df
{
enum MapShapeType
{
  GeometryType = 0,
  OverlayType,

  MapShapeTypeCount
};

class MapShape
{
public:
  virtual ~MapShape() = default;
  virtual void Prepare(ref_ptr<dp::TextureManager> /* textures */) const {}
  virtual void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                    ref_ptr<dp::TextureManager> textures) const = 0;
  virtual MapShapeType GetType() const { return MapShapeType::GeometryType; }

  void SetFeatureMinZoom(int minZoom) { m_minZoom = minZoom; }
  int GetFeatureMinZoom() const { return m_minZoom; }

  static m2::PointD ConvertToLocal(m2::PointD const & basePt, m2::PointD const & tileCenter, double scalar)
  {
    return (basePt - tileCenter) * scalar;
  }

private:
  int m_minZoom = 0;
};

using TMapShapes = std::vector<drape_ptr<MapShape>>;

class MapShapeMessage : public Message
{
public:
  explicit MapShapeMessage(TileKey const & key) : m_tileKey(key) {}

  TileKey const & GetKey() const { return m_tileKey; }

private:
  TileKey m_tileKey;
};

class TileReadStartMessage : public MapShapeMessage
{
public:
  explicit TileReadStartMessage(TileKey const & key) : MapShapeMessage(key) {}
  Type GetType() const override { return Type::TileReadStarted; }
  bool IsGraphicsContextDependent() const override { return true; }
};

class TileReadEndMessage : public MapShapeMessage
{
public:
  explicit TileReadEndMessage(TileKey const & key) : MapShapeMessage(key) {}
  Type GetType() const override { return Type::TileReadEnded; }
  bool IsGraphicsContextDependent() const override { return true; }
};

class MapShapeReadedMessage : public MapShapeMessage
{
public:
  MapShapeReadedMessage(TileKey const & key, TMapShapes && shapes) : MapShapeMessage(key), m_shapes(std::move(shapes))
  {}

  Type GetType() const override { return Type::MapShapeReaded; }
  bool IsGraphicsContextDependent() const override { return true; }
  TMapShapes const & GetShapes() { return m_shapes; }

private:
  TMapShapes m_shapes;
};

class OverlayMapShapeReadedMessage : public MapShapeReadedMessage
{
public:
  OverlayMapShapeReadedMessage(TileKey const & key, TMapShapes && shapes)
    : MapShapeReadedMessage(key, std::move(shapes))
  {}

  Type GetType() const override { return Message::Type::OverlayMapShapeReaded; }
};
}  // namespace df
