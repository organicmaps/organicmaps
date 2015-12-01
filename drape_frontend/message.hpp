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
    GetSelectedObject,
    GetMyPosition,
    AddRoute,
    CacheRouteSign,
    RemoveRoute,
    FlushRoute,
    FlushRouteSign,
    DeactivateRouteFollowing,
    UpdateMapStyle,
    InvalidateTextures,
    Invalidate
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
};

enum class MessagePriority
{
  Normal,
  High,
  UberHighSingleton
};

} // namespace df
