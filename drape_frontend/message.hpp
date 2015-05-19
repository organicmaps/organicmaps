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
    FinishReading,
    FlushTile,
    MapShapeReaded,
    UpdateReadManager,
    InvalidateRect,
    InvalidateReadManagerRect,
    Resize,
    ClearUserMarkLayer,
    ChangeUserMarkLayerVisibility,
    UpdateUserMarkLayer,
    GuiLayerRecached,
    GuiRecache,
    MyPositionShape,
    StorageInfoUpdated,
    StopRendering
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
};

enum class MessagePriority
{
  Normal,
  High
};

} // namespace df
