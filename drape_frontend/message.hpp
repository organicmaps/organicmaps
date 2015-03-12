#pragma once

namespace df
{

class Message
{
public:
  enum Type
  {
    Unknown,
    TileReadStarted,
    TileReadEnded,
    FlushTile,
    MapShapeReaded,
    UpdateModelView,
    UpdateReadManager,
    InvalidateRect,
    InvalidateReadManagerRect,
    Resize,
    ClearUserMarkLayer,
    ChangeUserMarkLayerVisibility,
    UpdateUserMarkLayer,
    GuiLayerRecached
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
};

} // namespace df
