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
    ClearUserMarkLayer,
    ChangeUserMarkLayerVisibility,
    UpdateUserMarkLayer,
    GuiLayerRecached,
    GuiRecache,
    GuiLayerLayout,
    MyPositionShape,
    CountryInfoUpdate,
    CountryStatusRecache,
    StopRendering,
    ChangeMyPostitionMode,
    CompassInfo,
    GpsInfo,
    FindVisiblePOI,
    SelectObject,
    GetMyPosition,
    AddRoute,
    RemoveRoute,
    FlushRoute
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
