#pragma once

#include "drape_frontend/message.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/feature_geometry_decl.hpp"
#include "drape/pointers.hpp"

namespace dp
{
  class Batcher;
  class TextureManager;
}

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
  virtual ~MapShape(){}
  virtual void Prepare(ref_ptr<dp::TextureManager> textures) const {}
  virtual void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const = 0;
  virtual MapShapeType GetType() const { return MapShapeType::GeometryType; }

  void SetFeatureMinZoom(int minZoom) { m_minZoom = minZoom; }
  int GetFeatureMinZoom() const { return m_minZoom; }

private:
  int m_minZoom = 0;
};

using TMapShapes = vector<drape_ptr<MapShape>>;

class MapShapeMessage : public Message
{
public:
  MapShapeMessage(TileKey const & key)
    : m_tileKey(key)
  {}

  TileKey const & GetKey() const { return m_tileKey; }

private:
  TileKey m_tileKey;
};

class TileReadStartMessage : public MapShapeMessage
{
public:
  TileReadStartMessage(TileKey const & key) : MapShapeMessage(key) {}
  Type GetType() const override { return Message::TileReadStarted; }
};

class TileReadEndMessage : public MapShapeMessage
{
public:
  TileReadEndMessage(TileKey const & key) : MapShapeMessage(key) {}
  Type GetType() const override { return Message::TileReadEnded; }
};

class MapShapeReadedMessage : public MapShapeMessage
{
public:
  MapShapeReadedMessage(TileKey const & key, TMapShapes && shapes)
    : MapShapeMessage(key), m_shapes(move(shapes))
  {}

  Type GetType() const override { return Message::MapShapeReaded; }
  TMapShapes const & GetShapes() { return m_shapes; }

private:
  TMapShapes m_shapes;
};

class OverlayMapShapeReadedMessage : public MapShapeReadedMessage
{
public:
  OverlayMapShapeReadedMessage(TileKey const & key, TMapShapes && shapes)
    : MapShapeReadedMessage(key, move(shapes))
  {}

  Type GetType() const override { return Message::OverlayMapShapeReaded; }
};

} // namespace df
