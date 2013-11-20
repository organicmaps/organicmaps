#pragma once

#include "tile_info.hpp"

namespace df
{
  class MapShape
  {
  public:
    virtual ~MapShape(){}
    virtual void Draw() const = 0;
  };

  class EngineContext
  {
  public:
    EngineContext();

    /// If you call this method, you may forget about shape.
    /// It will be proccessed and delete later
    void InsertShape(df::TileKey const & key, MapShape * shape) {}
  };
}
